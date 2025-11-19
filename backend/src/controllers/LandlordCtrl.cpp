#include "LandlordCtrl.h"
#include <fstream>
#include <algorithm>
#include <map>
#include <vector>
#include <cmath>

static std::string lower(std::string s){
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

// Helper: load reviews.json and compute per-landlord (sum, count)
static std::map<std::string, std::pair<double,int>> computeLandlordRatings()
{
    std::map<std::string, std::pair<double,int>> ratings;
    Json::Value reviewDb;
    std::ifstream reviewFile("data/reviews.json");
    if (reviewFile.good()) {
        reviewFile >> reviewDb;
    } else {
        std::ifstream reviewFile2("../data/reviews.json");
        if (reviewFile2.good()) {
            reviewFile2 >> reviewDb;
        } else {
            reviewDb["reviews"] = Json::Value(Json::arrayValue);
        }
    }

    for (const auto &review : reviewDb["reviews"]) {
        std::string landlordId = review["landlord_id"].asString();
        int rating = review["rating"].asInt();
        if (ratings.find(landlordId) == ratings.end()) {
            ratings[landlordId] = std::make_pair(0.0, 0);
        }
        ratings[landlordId].first += rating;
        ratings[landlordId].second += 1;
    }

    return ratings;
}

void LandlordCtrl::search(const drogon::HttpRequestPtr &req,
                          std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
    auto q = req->getParameter("name");
    std::string query = lower(q);

    Json::Value db;
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(dbPath_);
        if(!f.good()){
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k500InternalServerError);
            (*resp->getJsonObject())["error"] = "landlords database missing";
            cb(resp);
            return;
        }
        f >> db;
    }

    // Compute ratings map once and attach to results
    auto landlordRatings = computeLandlordRatings();

    Json::Value results(Json::arrayValue);
    const auto &arr = db["landlords"];
    for(const auto &ll : arr){
        std::string name = ll["name"].asString();
        if(query.empty() || lower(name).find(query) != std::string::npos){
            Json::Value landlord = ll;
            std::string landlordId = ll["landlord_id"].asString();
            double avgRating = 0.0;
            int reviewCount = 0;
            auto it = landlordRatings.find(landlordId);
            if(it != landlordRatings.end() && it->second.second > 0){
                avgRating = it->second.first / it->second.second;
                reviewCount = it->second.second;
            }
            landlord["average_rating"] = std::round(avgRating * 100.0) / 100.0;
            landlord["review_count"] = reviewCount;
            results.append(landlord);
        }
    }

    // Create a json object to send back
    Json::Value body(Json::objectValue);
    // define entry "results" to the array of names we just captured
    body["results"] = results;
    // format response to a drogon http response object
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);

    // now inside the reference (call back), but the response object inside
    cb(resp);
}

void LandlordCtrl::stats(const drogon::HttpRequestPtr &req,
                        std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
    Json::Value db;
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(dbPath_);
        if(!f.good()){
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k500InternalServerError);
            (*resp->getJsonObject())["error"] = "landlords database missing";
            cb(resp);
            return;
        }
        f >> db;
    }

    int landlordCount = 0;
    int propertyCount = 0;
    int unitCount = 0;

    const auto &arr = db["landlords"];
    for(const auto &ll : arr){
        landlordCount++;
        const auto &properties = ll["properties"];
        for(const auto &property : properties){
            propertyCount++;
            const auto &units = property["unit_details"];
            for(const auto &unit : units){
                unitCount++;
            }
        }
    }

    Json::Value body(Json::objectValue);
    body["landlords"] = landlordCount;
    body["properties"] = propertyCount;
    body["units"] = unitCount;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    cb(resp);
}

void LandlordCtrl::leaderboard(const drogon::HttpRequestPtr &req,
                                std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
    // Load landlords data
    Json::Value landlordDb;
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(dbPath_);
        if(!f.good()){
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k500InternalServerError);
            (*resp->getJsonObject())["error"] = "landlords database missing";
            cb(resp);
            return;
        }
        f >> landlordDb;
    }

    // Compute ratings map once (sum,count) using helper
    auto landlordRatings = computeLandlordRatings();

    // Create leaderboard entries
    Json::Value results(Json::arrayValue);
    std::vector<std::pair<std::string, double>> sortedLandlords;

    for (const auto &ll : landlordDb["landlords"]) {
        std::string landlordId = ll["landlord_id"].asString();
        double avgRating = 0.0;
        int reviewCount = 0;

        if (landlordRatings.find(landlordId) != landlordRatings.end()) {
            avgRating = landlordRatings[landlordId].first / landlordRatings[landlordId].second;
            reviewCount = landlordRatings[landlordId].second;
        }

        // Create leaderboard entry
        Json::Value entry = ll;
        entry["average_rating"] = std::round(avgRating * 100.0) / 100.0; // Round to 2 decimal places
        entry["review_count"] = reviewCount;
        sortedLandlords.push_back({landlordId, avgRating});
        results.append(entry);
    }

    // Sort results by average rating (highest to lowest)
    // We need to reorder the results array
    std::sort(sortedLandlords.begin(), sortedLandlords.end(), 
              [](const auto &a, const auto &b) { return a.second > b.second; });

    Json::Value sortedResults(Json::arrayValue);
    for (const auto &sorted : sortedLandlords) {
        for (const auto &entry : results) {
            if (entry["landlord_id"].asString() == sorted.first) {
                sortedResults.append(entry);
                break;
            }
        }
    }

    Json::Value body(Json::objectValue);
    body["leaderboard"] = sortedResults;
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    cb(resp);
}
