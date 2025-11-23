#include "ReviewCtrl.h"
#include "SupabaseHelper.h"
#include <fstream>
#include <chrono>
#include <random>

void ReviewCtrl::submit(const drogon::HttpRequestPtr &req,
                       std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
    // Parse request body
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "Invalid JSON body";
        callback(resp);
        return;
    }

    // Validate required fields
    if (!(*json)["landlord_id"].isString() || 
        !(*json)["rating"].isInt() ||
        !(*json)["title"].isString() ||
        !(*json)["review"].isString()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "Missing required fields";
        callback(resp);
        return;
    }

    // Validate rating range (1-5)
    int rating = (*json)["rating"].asInt();
    if(rating < 1 || rating > 5) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "Rating must be between 1 and 5";
        callback(resp);
        return;
    }

    // Create new review object
    Json::Value review;
    review["landlord_id"] = (*json)["landlord_id"];
    review["rating"] = rating;
    review["title"] = (*json)["title"];
    review["review"] = (*json)["review"];
    
    // Generate a unique ID (retry if collision)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);
    std::string reviewId;
    int attempts = 0;
    do {
        reviewId = std::to_string(dis(gen));
        attempts++;
        // Check if ID exists in Supabase (simple check - in production use UUID)
        // For now, just use timestamp + random to reduce collision chance
        if(attempts > 5) {
            // Fallback: use timestamp + random
            auto now = std::chrono::system_clock::now();
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            reviewId = std::to_string(now_ms % 90000 + 10000);
            break;
        }
    } while(attempts < 10);
    review["id"] = reviewId;

    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now_c));
    review["created_at"] = timeStr;

    // Insert to Supabase database (retry with new ID if collision)
    std::string supabaseErr;
    bool inserted = false;
    for(int retry = 0; retry < 3 && !inserted; retry++) {
        if(SupabaseHelper::insertReview(review["id"].asString(),
                                        review["landlord_id"].asString(),
                                        review["rating"].asInt(),
                                        review["title"].asString(),
                                        review["review"].asString(),
                                        review["created_at"].asString(),
                                        supabaseErr)) {
            inserted = true;
        } else if(supabaseErr.find("duplicate key") != std::string::npos || 
                  supabaseErr.find("23505") != std::string::npos) {
            // ID collision - generate new ID
            auto now = std::chrono::system_clock::now();
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            review["id"] = std::to_string(now_ms % 90000 + 10000);
        } else {
            // Other error - break and return error
            break;
        }
    }
    
    if(!inserted) {
        LOG_ERROR << "Supabase review insert failed: " << supabaseErr;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "failed to save review to database: " + supabaseErr;
        callback(resp);
        return;
    }

    // Return success response
    auto resp = drogon::HttpResponse::newHttpJsonResponse(review);
    callback(resp);
}

void ReviewCtrl::getForLandlord(const drogon::HttpRequestPtr &req,
                               std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                               const std::string &landlordId) {
    // Get reviews from Supabase database
    Json::Value reviewsArray;
    std::string err;
    if(!SupabaseHelper::getReviewsForLandlord(landlordId, reviewsArray, err)) {
        LOG_ERROR << "Failed to get reviews for landlord " << landlordId << ": " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "failed to load reviews";
        callback(resp);
        return;
    }

    // Return reviews
    Json::Value response(Json::objectValue);
    response["reviews"] = reviewsArray;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void ReviewCtrl::submitReport(const drogon::HttpRequestPtr &req,
                              std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "Invalid JSON body";
        callback(resp);
        return;
    }

    // Required fields: review_id, title, review, reason, reported_by
    if (!(*json)["review_id"].isString() || !(*json)["title"].isString() || !(*json)["review"].isString() || !(*json)["reason"].isString()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "Missing required fields";
        callback(resp);
        return;
    }

    // Generate ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    std::string reportId = std::string("RPT") + std::to_string(dis(gen));

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now_c));

    // Insert into Supabase
    std::string err;
    if(!SupabaseHelper::insertReportedReview(reportId,
                                             (*json)["review_id"].asString(),
                                             (*json)["title"].asString(),
                                             (*json)["review"].asString(),
                                             (*json)["reason"].asString(),
                                             (*json).get("reported_by", Json::Value("anonymous")).asString(),
                                             timeStr,
                                             err)) {
        LOG_ERROR << "Failed to insert reported review: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "Failed to save report: " + err;
        callback(resp);
        return;
    }

    Json::Value rep(Json::objectValue);
    rep["id"] = reportId;
    rep["review_id"] = (*json)["review_id"];
    rep["title"] = (*json)["title"];
    rep["review"] = (*json)["review"];
    rep["reason"] = (*json)["reason"];
    rep["reported_by"] = (*json).get("reported_by", Json::Value("anonymous")).asString();
    rep["created_at"] = timeStr;
    rep["status"] = "pending";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(rep);
    callback(resp);
}