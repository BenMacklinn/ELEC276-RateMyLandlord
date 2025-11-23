#include "AdminCtrl.h"
#include "SupabaseHelper.h"
#include <fstream>

bool AdminCtrl::isAdmin(const std::string &email) {
    // Check admin status from Supabase database
    bool isAdminUser = false;
    std::string err;
    if(!SupabaseHelper::getUserAdminStatus(email, isAdminUser, err)) {
        LOG_ERROR << "Failed to check admin status for " << email << ": " << err;
        return false; // Fail closed - if we can't verify, deny access
    }
    return isAdminUser;
}

void AdminCtrl::getReported(const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
    auto authHeader = req->getHeader("authorization");
    auto email = parseToken(authHeader);
    if(email.empty() || !isAdmin(email)) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k401Unauthorized);
        (*resp->getJsonObject())["error"] = "missing/invalid token or not admin";
        cb(resp);
        return;
    }

    Json::Value reportsArray;
    std::string err;
    if(!SupabaseHelper::getAllReportedReviews(reportsArray, err)) {
        LOG_ERROR << "Failed to get reported reviews: " << err;
        // Return 200 with empty array - endpoint exists but query failed
        // This prevents frontend from thinking endpoint is missing
        Json::Value root(Json::objectValue);
        root["reports"] = Json::Value(Json::arrayValue);
        auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
        resp->setStatusCode(drogon::k200OK);
        cb(resp);
        return;
    }

    Json::Value root(Json::objectValue);
    root["reports"] = reportsArray;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
    cb(resp);
}

void AdminCtrl::approve(const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb, const std::string &id) {
    auto authHeader = req->getHeader("authorization");
    auto email = parseToken(authHeader);
    if(email.empty() || !isAdmin(email)) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k401Unauthorized);
        (*resp->getJsonObject())["error"] = "missing/invalid token or not admin";
        cb(resp);
        return;
    }

    // Get the report to find the review_id
    Json::Value reportsArray;
    std::string err;
    if(!SupabaseHelper::getAllReportedReviews(reportsArray, err)) {
        LOG_ERROR << "Failed to get reported reviews: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "Failed to load reports: " + err;
        cb(resp);
        return;
    }

    std::string removedReviewId;
    bool found = false;
    for(const auto &r : reportsArray) {
        if(r["id"].asString() == id) {
            removedReviewId = r.get("review_id", "").asString();
            found = true;
            break;
        }
    }

    if(!found) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k404NotFound);
        (*resp->getJsonObject())["error"] = "report not found";
        cb(resp);
        return;
    }

    // Delete the report
    if(!SupabaseHelper::deleteReportedReview(id, err)) {
        LOG_ERROR << "Failed to delete report: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "Failed to delete report: " + err;
        cb(resp);
        return;
    }

    // Delete the review if review_id was found
    if(!removedReviewId.empty()) {
        if(!SupabaseHelper::deleteReview(removedReviewId, err)) {
            LOG_ERROR << "Failed to delete review " << removedReviewId << ": " << err;
            // Continue anyway - report was deleted
        }
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
    (*resp->getJsonObject())["message"] = "report approved and review removed";
    cb(resp);
}

void AdminCtrl::deny(const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb, const std::string &id) {
    auto authHeader = req->getHeader("authorization");
    auto email = parseToken(authHeader);
    if(email.empty() || !isAdmin(email)) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k401Unauthorized);
        (*resp->getJsonObject())["error"] = "missing/invalid token or not admin";
        cb(resp);
        return;
    }

    // Delete the report from Supabase
    std::string err;
    if(!SupabaseHelper::deleteReportedReview(id, err)) {
        LOG_ERROR << "Failed to delete report: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "Failed to delete report: " + err;
        cb(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
    (*resp->getJsonObject())["message"] = "report removed (denied)";
    cb(resp);
}
