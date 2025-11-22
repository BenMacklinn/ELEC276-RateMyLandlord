#include "LandlordCtrl.h"
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

    Json::Value db;
    {
        std::lock_guard<std::mutex> lk(mu_);

        // Load existing requests file if present
        std::ifstream f(requestDbPath_);
        if (f.good()) {
            f >> db;
        } else {
            db["requests"] = Json::Value(Json::arrayValue);
        }

        if (!db.isMember("requests") || !db["requests"].isArray()) {
            db["requests"] = Json::Value(Json::arrayValue);
        }

        // Compute next id
        int nextId = 1;
        for (const auto &reqItem : db["requests"]) {
            if (reqItem.isMember("id")) {
                nextId = std::max(nextId, reqItem["id"].asInt() + 1);
            }
        }

        // Timestamp
        std::time_t now = std::time(nullptr);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));

        Json::Value newReq(Json::objectValue);
        newReq["id"]             = nextId;
        newReq["landlord_name"]  = landlordName;
        newReq["landlord_email"] = landlordEmail;
        newReq["landlord_phone"] = landlordPhone;
        newReq["details"]        = details;

        // Save full properties array (no extra top-level property_* fields)
        newReq["properties"]     = properties;

        // who submitted the request
        newReq["user_name"]      = requesterName;
        newReq["user_email"]     = requesterEmail;
        newReq["created_at"]     = buf;


        db["requests"].append(newReq);

        // Save back to file
        std::ofstream out(requestDbPath_);
        if (!out.good()) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k500InternalServerError);
            (*resp->getJsonObject())["error"] = "Failed to write landlord_requests database";
            cb(resp);
            return;
        }
        out << db;
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
    Json::Value db;
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(requestDbPath_);
        if (f.good()) {
            f >> db;
        } else {
            db["requests"] = Json::Value(Json::arrayValue);
        }
    }

    if (!db.isMember("requests") || !db["requests"].isArray()) {
        db["requests"] = Json::Value(Json::arrayValue);
    }

    Json::Value body(Json::objectValue);
    body["requests"] = db["requests"];
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

    Json::Value requestsDb;
    Json::ArrayIndex foundIndex = Json::ArrayIndex(-1);

    {
        std::lock_guard<std::mutex> lk(mu_);

        std::ifstream rf(requestDbPath_);
        if (rf.good()) {
            rf >> requestsDb;
        } else {
            requestsDb["requests"] = Json::Value(Json::arrayValue);
        }

        if (!requestsDb.isMember("requests") || !requestsDb["requests"].isArray()) {
            requestsDb["requests"] = Json::Value(Json::arrayValue);
        }

        auto &arr = requestsDb["requests"];
        for (Json::ArrayIndex i = 0; i < arr.size(); ++i) {
            if (arr[i]["id"].asInt() == requestId) {
                foundIndex = i;
                break;
            }
        }

        if (foundIndex == Json::ArrayIndex(-1)) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k404NotFound);
            (*resp->getJsonObject())["error"] = "Request not found";
            cb(resp);
            return;
        }

        // Remove this request from the array entirely
        Json::Value newArr(Json::arrayValue);
        for (Json::ArrayIndex i = 0; i < arr.size(); ++i) {
            if (i != foundIndex) {
                newArr.append(arr[i]);
            }
        }
        requestsDb["requests"] = newArr;

        std::ofstream out(requestDbPath_);
        out << requestsDb;
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
    Json::Value requestsDb;
    Json::Value landlordsDb;
    Json::ArrayIndex foundIndex = Json::ArrayIndex(-1);
    Json::Value reqCopy;

    {
        std::lock_guard<std::mutex> lk(mu_);

        // load landlord_requests.json
        std::ifstream rf(requestDbPath_);
        if (rf.good()) {
            rf >> requestsDb;
        } else {
            requestsDb["requests"] = Json::Value(Json::arrayValue);
        }

        if (!requestsDb.isMember("requests") || !requestsDb["requests"].isArray()) {
            requestsDb["requests"] = Json::Value(Json::arrayValue);
        }

        auto &arr = requestsDb["requests"];
        for (Json::ArrayIndex i = 0; i < arr.size(); ++i) {
            if (arr[i]["id"].asInt() == requestId) {
                foundIndex = i;
                reqCopy = arr[i];
                break;
            }
        }

        if (foundIndex == Json::ArrayIndex(-1)) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k404NotFound);
            (*resp->getJsonObject())["error"] = "Request not found";
            cb(resp);
            return;
        }

        // load landlords.json
        std::ifstream lf(dbPath_);
        if (lf.good()) {
            lf >> landlordsDb;
        } else {
            landlordsDb["landlords"] = Json::Value(Json::arrayValue);
        }

        if (!landlordsDb.isMember("landlords") || !landlordsDb["landlords"].isArray()) {
            landlordsDb["landlords"] = Json::Value(Json::arrayValue);
        }

        // generate new landlord id LL###
        int maxId = 0;
        for (auto &ll : landlordsDb["landlords"]) {
            auto idStr = ll["landlord_id"].asString();
            if (idStr.rfind("LL", 0) == 0) {
                int num = std::stoi(idStr.substr(2));
                if (num > maxId) maxId = num;
            }
        }
        int newId = maxId + 1;

        char buf[16];
        snprintf(buf, sizeof(buf), "LL%03d", newId);

        Json::Value newLL(Json::objectValue);
        newLL["landlord_id"] = buf;
        newLL["name"] = reqCopy["landlord_name"].asString();

        // contact = landlord's contact details from the request
        Json::Value contact(Json::objectValue);
        contact["email"] = reqCopy["landlord_email"].asString();
        contact["phone"] = reqCopy["landlord_phone"].asString();
        newLL["contact"] = contact;

        // Build properties from the request.
        Json::Value props(Json::arrayValue);
        Json::Value propsFromReq = reqCopy["properties"];

        if (propsFromReq.isArray() && propsFromReq.size() > 0) {
            int propIndex = 0;
            for (const auto &rp : propsFromReq) {
                std::string street = rp["property_address"].asString();
                if (street.empty()) continue;

                Json::Value p(Json::objectValue);

                // property_id e.g. P1_1, P1_2...
                char pid[32];
                snprintf(pid, sizeof(pid), "P%d_%d", newId, ++propIndex);
                p["property_id"] = pid;

                Json::Value a(Json::objectValue);
                a["street"] = street;
                a["city"]   = rp["property_city"].asString();
                a["state"]  = rp["property_state"].asString();
                a["zip"]    = rp["property_zip"].asString();
                p["address"] = a;

                Json::Value units(Json::arrayValue);
                std::string unitNumber = rp["unit_number"].asString();
                if (!unitNumber.empty()) {
                    Json::Value u(Json::objectValue);
                    u["unit_number"] = unitNumber;
                    u["bedrooms"]    = rp["unit_bedrooms"].asInt();
                    u["bathrooms"]   = rp["unit_bathrooms"].asInt();
                    u["rent"]        = rp["unit_rent"].asInt();
                    units.append(u);
                }
                p["unit_details"] = units;

                props.append(p);
            }
        } else {
            // Fallback to legacy single-property fields
            Json::Value p(Json::objectValue);
            std::string street = reqCopy["property_address"].asString();
            if (!street.empty()) {
                p["property_id"] = "P" + std::to_string(newId);

                Json::Value a(Json::objectValue);
                a["street"] = street;
                a["city"]   = reqCopy["property_city"].asString();
                a["state"]  = reqCopy["property_state"].asString();
                a["zip"]    = reqCopy["property_zip"].asString();
                p["address"] = a;

                Json::Value units(Json::arrayValue);
                std::string unitNumber = reqCopy["unit_number"].asString();
                if (!unitNumber.empty()) {
                    Json::Value u(Json::objectValue);
                    u["unit_number"] = unitNumber;
                    u["bedrooms"]    = reqCopy["unit_bedrooms"].asInt();
                    u["bathrooms"]   = reqCopy["unit_bathrooms"].asInt();
                    u["rent"]        = reqCopy["unit_rent"].asInt();
                    units.append(u);
                }
                p["unit_details"] = units;

                props.append(p);
            }
        }

        newLL["properties"] = props;
        landlordsDb["landlords"].append(newLL);

        // Remove the request completely instead of marking it approved
        Json::Value removed;
        arr.removeIndex(foundIndex, &removed);

        // write both files
        std::ofstream outReq(requestDbPath_);
        outReq << requestsDb;

        std::ofstream outLL(dbPath_);
        outLL << landlordsDb;
    }

    Json::Value body(Json::objectValue);
    body["ok"] = true;
    body["id"] = requestId;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    cb(resp);
}
