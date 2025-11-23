#include "LandlordCtrl.h"
#include "SupabaseHelper.h"
#include <fstream>
#include <algorithm>
#include <map>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdio> 

static std::string lower(std::string s){
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

// Helper: load reviews from Supabase and compute per-landlord (sum, count)
static std::map<std::string, std::pair<double,int>> computeLandlordRatings()
{
    std::map<std::string, std::pair<double,int>> ratings;
    
    // Get all reviews from Supabase
    Json::Value reviewsArray;
    std::string err;
    if(!SupabaseHelper::getAllReviews(reviewsArray, err)) {
        LOG_ERROR << "Failed to load reviews for rating computation: " << err;
        return ratings; // Return empty map if Supabase fails
    }

    // Compute ratings from Supabase reviews
    for (const auto &review : reviewsArray) {
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

    // Get all landlords from Supabase
    Json::Value landlordsJson;
    std::string err;
    if(!SupabaseHelper::getAllLandlords(landlordsJson, err)) {
        LOG_ERROR << "Failed to get landlords from Supabase: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "failed to load landlords: " + err;
        cb(resp);
        return;
    }

    // Compute ratings map once and attach to results
    auto landlordRatings = computeLandlordRatings();

    Json::Value results(Json::arrayValue);
    for(const auto &ll : landlordsJson){
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
    int landlordCount = 0;
    int propertyCount = 0;
    int unitCount = 0;
    std::string err;
    
    if(!SupabaseHelper::getLandlordStats(landlordCount, propertyCount, unitCount, err)) {
        LOG_ERROR << "Failed to get landlord stats from Supabase: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "failed to load stats: " + err;
        cb(resp);
        return;
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
    // Load landlords data from Supabase
    Json::Value landlordsArray;
    std::string err;
    if(!SupabaseHelper::getAllLandlords(landlordsArray, err)) {
        LOG_ERROR << "Failed to get landlords from Supabase: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "failed to load landlords: " + err;
        cb(resp);
        return;
    }

    // Compute ratings map once (sum,count) using helper
    auto landlordRatings = computeLandlordRatings();

    // Create leaderboard entries
    Json::Value results(Json::arrayValue);
    std::vector<std::pair<std::string, double>> sortedLandlords;

    for (const auto &ll : landlordsArray) {
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

void LandlordCtrl::submitRequest(const drogon::HttpRequestPtr &req,
                            std::function<void (const drogon::HttpResponsePtr &)> &&cb)
{
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "Request body must be JSON";
        cb(resp);
        return;
    }

    // Landlord contact info (required)
    std::string landlordName   = (*json)["landlord_name"].asString();
    std::string landlordEmail  = (*json)["landlord_email"].asString();
    std::string landlordPhone  = (*json)["landlord_phone"].asString();

    // Requester info (auto-filled by frontend from logged-in user)
    std::string requesterName  = (*json)["user_name"].asString();
    std::string requesterEmail = (*json)["user_email"].asString();

    std::string details        = (*json)["details"].asString();

    // Enforce mandatory landlord contact
    if (landlordName.empty() || landlordEmail.empty() || landlordPhone.empty()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] =
            "landlord_name, landlord_email, and landlord_phone are required";
        cb(resp);
        return;
    }

    // Normalized properties array
    Json::Value properties(Json::arrayValue);

    // Preferred path: "properties" is an array from the frontend
    if (json->isMember("properties") && (*json)["properties"].isArray() && (*json)["properties"].size() > 0) {
        const auto &props = (*json)["properties"];
        for (const auto &p : props) {
            std::string propertyAddress = p["property_address"].asString();
            std::string propertyCity    = p["property_city"].asString();
            std::string propertyState   = p["property_state"].asString();
            std::string propertyZip     = p["property_zip"].asString();

            std::string unitNumber      = p["unit_number"].asString();
            int unitBedrooms            = p["unit_bedrooms"].asInt();
            int unitBathrooms           = p["unit_bathrooms"].asInt();
            int unitRent                = p["unit_rent"].asInt();

            if (propertyAddress.empty() ||
                propertyCity.empty() ||
                propertyState.empty() ||
                propertyZip.empty() ||
                unitNumber.empty())
            {
                auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
                resp->setStatusCode(drogon::k400BadRequest);
                (*resp->getJsonObject())["error"] =
                    "Each property must include address, city/state/zip, and unit details";
                cb(resp);
                return;
            }

            Json::Value prop(Json::objectValue);
            prop["property_address"] = propertyAddress;
            prop["property_city"]    = propertyCity;
            prop["property_state"]   = propertyState;
            prop["property_zip"]     = propertyZip;
            prop["unit_number"]      = unitNumber;
            prop["unit_bedrooms"]    = unitBedrooms;
            prop["unit_bathrooms"]   = unitBathrooms;
            prop["unit_rent"]        = unitRent;
            properties.append(prop);
        }
    } else {
        // Backwards-compatible path: single-property fields
        std::string propertyInfo   = (*json)["property_address"].asString();
        std::string propertyCity   = (*json)["property_city"].asString();
        std::string propertyState  = (*json)["property_state"].asString();
        std::string propertyZip    = (*json)["property_zip"].asString();

        std::string unitNumber     = (*json)["unit_number"].asString();
        int unitBedrooms           = (*json)["unit_bedrooms"].asInt();
        int unitBathrooms          = (*json)["unit_bathrooms"].asInt();
        int unitRent               = (*json)["unit_rent"].asInt();

        if (propertyInfo.empty() ||
            propertyCity.empty() ||
            propertyState.empty() ||
            propertyZip.empty() ||
            unitNumber.empty())
        {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k400BadRequest);
            (*resp->getJsonObject())["error"] =
                "At least one property with address, city/state/zip, and unit details is required";
            cb(resp);
            return;
        }

        Json::Value prop(Json::objectValue);
        prop["property_address"] = propertyInfo;
        prop["property_city"]    = propertyCity;
        prop["property_state"]   = propertyState;
        prop["property_zip"]     = propertyZip;
        prop["unit_number"]      = unitNumber;
        prop["unit_bedrooms"]    = unitBedrooms;
        prop["unit_bathrooms"]   = unitBathrooms;
        prop["unit_rent"]        = unitRent;
        properties.append(prop);
    }

    // Insert request into Supabase
    int requestId = 0;
    std::string err;
    if(!SupabaseHelper::insertLandlordRequest(requestId, landlordName, landlordEmail, landlordPhone,
                                               requesterName, requesterEmail, details, properties, err)) {
        LOG_ERROR << "Failed to insert landlord request: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "Failed to save request: " + err;
        cb(resp);
        return;
    }

    Json::Value body(Json::objectValue);
    body["ok"] = true;
    body["message"] = "Landlord request submitted";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    cb(resp);
}


void LandlordCtrl::listRequests(const drogon::HttpRequestPtr &req,
                                std::function<void (const drogon::HttpResponsePtr &)> &&cb)
{
    Json::Value requestsArray;
    std::string err;
    if(!SupabaseHelper::getAllLandlordRequests(requestsArray, err)) {
        LOG_ERROR << "Failed to get landlord requests: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "Failed to load requests: " + err;
        cb(resp);
        return;
    }

    Json::Value body(Json::objectValue);
    body["requests"] = requestsArray;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    cb(resp);
}

void LandlordCtrl::rejectRequest(const drogon::HttpRequestPtr &req,
                                 std::function<void (const drogon::HttpResponsePtr &)> &&cb,
                                 int requestId)
{
    std::string reason;
    if (auto json = req->getJsonObject()) {
        if ((*json).isMember("reason"))
            reason = (*json)["reason"].asString();
    }

    // Delete request from Supabase
    std::string err;
    if(!SupabaseHelper::deleteLandlordRequest(requestId, err)) {
        LOG_ERROR << "Failed to delete landlord request " << requestId << ": " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "Failed to delete request: " + err;
        cb(resp);
        return;
    }

    Json::Value body(Json::objectValue);
    body["ok"] = true;
    body["id"] = requestId;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    cb(resp);
}

void LandlordCtrl::approveRequest(const drogon::HttpRequestPtr &req,
                                  std::function<void (const drogon::HttpResponsePtr &)> &&cb,
                                  int requestId)
{
    // Get request from Supabase
    Json::Value requestsArray;
    std::string err;
    if(!SupabaseHelper::getAllLandlordRequests(requestsArray, err)) {
        LOG_ERROR << "Failed to get landlord requests: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "Failed to load requests: " + err;
        cb(resp);
        return;
    }

    Json::Value reqCopy;
    bool found = false;
    for(const auto &r : requestsArray) {
        if(r["id"].asInt() == requestId) {
            reqCopy = r;
            found = true;
            break;
        }
    }

    if(!found) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k404NotFound);
        (*resp->getJsonObject())["error"] = "Request not found";
        cb(resp);
        return;
    }

    // Get all landlords to find max ID
    Json::Value landlordsArray;
    if(!SupabaseHelper::getAllLandlords(landlordsArray, err)) {
        LOG_ERROR << "Failed to get landlords: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "Failed to load landlords: " + err;
        cb(resp);
        return;
    }

    // Generate new landlord ID
    int maxId = 0;
    for(const auto &ll : landlordsArray) {
        std::string idStr = ll["landlord_id"].asString();
        if(idStr.rfind("LL", 0) == 0 && idStr.length() >= 3) {
            try {
                int num = std::stoi(idStr.substr(2));
                if(num > maxId) maxId = num;
            } catch(const std::exception &e) {
                // Skip invalid IDs
                continue;
            }
        }
    }
    int newId = maxId + 1;
    char buf[16];
    snprintf(buf, sizeof(buf), "LL%03d", newId);
    std::string landlordId = buf;

    // Build properties array from request
    Json::Value props(Json::arrayValue);
    Json::Value propsFromReq = reqCopy["properties"];

    if(propsFromReq.isArray() && propsFromReq.size() > 0) {
        int propIndex = 0;
        for(const auto &rp : propsFromReq) {
            Json::Value p(Json::objectValue);
            char pid[32];
            snprintf(pid, sizeof(pid), "P%d_%d", newId, ++propIndex);
            p["property_id"] = pid;

            Json::Value a(Json::objectValue);
            a["street"] = rp["property_address"].asString();
            a["city"] = rp["property_city"].asString();
            a["province"] = rp.get("property_state", rp.get("property_province", "")).asString();
            a["zip"] = rp["property_zip"].asString();
            p["address"] = a;

            Json::Value units(Json::arrayValue);
            Json::Value u(Json::objectValue);
            u["unit_number"] = rp["unit_number"].asString();
            u["bedrooms"] = rp["unit_bedrooms"].asInt();
            u["bathrooms"] = rp["unit_bathrooms"].asInt();
            u["rent"] = rp["unit_rent"].asInt();
            units.append(u);
            p["unit_details"] = units;

            props.append(p);
        }
    } else {
        // Fallback to legacy single-property fields
        Json::Value p(Json::objectValue);
        p["property_id"] = "P" + std::to_string(newId);
        Json::Value a(Json::objectValue);
        a["street"] = reqCopy["property_address"].asString();
        a["city"] = reqCopy["property_city"].asString();
        a["province"] = reqCopy.get("property_state", reqCopy.get("property_province", "")).asString();
        a["zip"] = reqCopy["property_zip"].asString();
        p["address"] = a;
        Json::Value units(Json::arrayValue);
        Json::Value u(Json::objectValue);
        u["unit_number"] = reqCopy["unit_number"].asString();
        u["bedrooms"] = reqCopy["unit_bedrooms"].asInt();
        u["bathrooms"] = reqCopy["unit_bathrooms"].asInt();
        u["rent"] = reqCopy["unit_rent"].asInt();
        units.append(u);
        p["unit_details"] = units;
        props.append(p);
    }

    // Insert landlord into Supabase
    if(!SupabaseHelper::insertLandlord(landlordId, reqCopy["landlord_name"].asString(),
                                       reqCopy["landlord_email"].asString(),
                                       reqCopy["landlord_phone"].asString(),
                                       props, err)) {
        LOG_ERROR << "Failed to insert landlord: " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "Failed to create landlord: " + err;
        cb(resp);
        return;
    }

    // Delete the request
    if(!SupabaseHelper::deleteLandlordRequest(requestId, err)) {
        LOG_ERROR << "Failed to delete request: " << err;
        // Continue anyway - landlord was created
    }

    Json::Value body(Json::objectValue);
    body["ok"] = true;
    body["id"] = requestId;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    cb(resp);
}
