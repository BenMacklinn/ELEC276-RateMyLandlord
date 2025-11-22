#include "ReviewCtrl.h"
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

    // Load existing reviews
    Json::Value db;
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(dbPath_);
        if (f.good()) {
            f >> db;
        } else {
            db["reviews"] = Json::Value(Json::arrayValue);
        }
    }

    // Create new review object
    Json::Value review;
    review["landlord_id"] = (*json)["landlord_id"];
    review["rating"] = (*json)["rating"];
    review["title"] = (*json)["title"];
    review["review"] = (*json)["review"];
    
    // Generate a unique ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);
    review["id"] = std::to_string(dis(gen));

    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now_c));
    review["created_at"] = timeStr;

    // Add to database
    db["reviews"].append(review);

    // Save back to file
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ofstream f(dbPath_);
        f << db.toStyledString();
    }

    // Return success response
    auto resp = drogon::HttpResponse::newHttpJsonResponse(review);
    callback(resp);
}

void ReviewCtrl::getForLandlord(const drogon::HttpRequestPtr &req,
                               std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                               const std::string &landlordId) {
    Json::Value db;
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(dbPath_);
        if (!f.good()) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k500InternalServerError);
            (*resp->getJsonObject())["error"] = "Reviews database missing";
            callback(resp);
            return;
        }
        f >> db;
    }

    // Filter reviews for this landlord
    Json::Value results(Json::arrayValue);
    for (const auto &review : db["reviews"]) {
        if (review["landlord_id"].asString() == landlordId) {
            results.append(review);
        }
    }

    // Return filtered reviews
    Json::Value response(Json::objectValue);
    response["reviews"] = results;
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

    Json::Value db;
    std::string reportedPath;
    // try several plausible locations
    const std::vector<std::string> candidates = {"data/reported.json", "../data/reported.json", "backend/data/reported.json"};
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f;
        for (const auto &c : candidates) {
            f.close();
            f.open(c);
            if (f.good()) { reportedPath = c; break; }
        }
        if (!reportedPath.empty()) {
            std::ifstream fr(reportedPath);
            fr >> db;
        } else {
            db["reports"] = Json::Value(Json::arrayValue);
        }
    }

    Json::Value rep;
    rep["review_id"] = (*json)["review_id"];
    rep["title"] = (*json)["title"];
    rep["review"] = (*json)["review"];
    rep["reason"] = (*json)["reason"];
    rep["reported_by"] = (*json).get("reported_by", Json::Value("anonymous")).asString();

    // generate id
    static int counter = 1000;
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        rep["id"] = std::string("RPT") + std::to_string(dis(gen));
    }

    // timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now_c));
    rep["created_at"] = timeStr;
    rep["status"] = "pending";

    db["reports"].append(rep);

    {
        std::lock_guard<std::mutex> lk(mu_);
        if (reportedPath.empty()) reportedPath = "data/reported.json";
        std::ofstream out(reportedPath, std::ios::trunc);
        out << db.toStyledString();
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(rep);
    callback(resp);
}