#include "AdminCtrl.h"
#include <fstream>

bool AdminCtrl::isAdmin(const std::string &email) {
    // load users.json and check admin flag
    Json::Value arr(Json::arrayValue);
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(usersPath_);
        if(f.good()) f >> arr;
    }

    for(const auto &u : arr) {
        if(u["email"].asString() == email) {
            return u.get("admin", 0).asInt() == 1;
        }
    }
    return false;
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

    Json::Value root(Json::objectValue);
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(reportedPath_);
        if(f.good()) {
            f >> root;
        } else {
            root["reports"] = Json::Value(Json::arrayValue);
        }
    }

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

    Json::Value root;
    std::string removedReviewId;
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(reportedPath_);
        if(f.good()) f >> root;
        else root["reports"] = Json::Value(Json::arrayValue);

        bool found = false;
        Json::Value newReports(Json::arrayValue);
        for(const auto &r : root["reports"]) {
            if(r["id"].asString() == id) {
                // remove this report (approved)
                removedReviewId = r.get("review_id", "").asString();
                found = true;
                continue;
            }
            newReports.append(r);
        }
        if(found) {
            root["reports"] = newReports;
            std::ofstream out(reportedPath_, std::ios::trunc);
            out << root.toStyledString();
        } else {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k404NotFound);
            (*resp->getJsonObject())["error"] = "report not found";
            cb(resp);
            return;
        }
    }

    // If we removed a report, also remove the referenced review from reviews.json
    if(!removedReviewId.empty()) {
        Json::Value reviewsRoot;
        {
            std::lock_guard<std::mutex> lk(mu_);
            std::ifstream rf(reviewsPath_);
            if(rf.good()) rf >> reviewsRoot;
            else reviewsRoot["reviews"] = Json::Value(Json::arrayValue);

            Json::Value newReviews(Json::arrayValue);
            for(const auto &rv : reviewsRoot["reviews"]) {
                if(rv.get("id", "").asString() == removedReviewId) {
                    continue; // drop this review
                }
                newReviews.append(rv);
            }
            reviewsRoot["reviews"] = newReviews;
            std::ofstream rfout(reviewsPath_, std::ios::trunc);
            rfout << reviewsRoot.toStyledString();
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

    Json::Value root;
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(reportedPath_);
        if(f.good()) f >> root;
        else root["reports"] = Json::Value(Json::arrayValue);

        bool found = false;
        Json::Value newReports(Json::arrayValue);
        for(const auto &r : root["reports"]) {
            if(r["id"].asString() == id) {
                // drop the report (denied)
                found = true;
                continue;
            }
            newReports.append(r);
        }
        if(found) {
            root["reports"] = newReports;
            std::ofstream out(reportedPath_, std::ios::trunc);
            out << root.toStyledString();
        } else {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k404NotFound);
            (*resp->getJsonObject())["error"] = "report not found";
            cb(resp);
            return;
        }
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
    (*resp->getJsonObject())["message"] = "report removed (denied)";
    cb(resp);
}
