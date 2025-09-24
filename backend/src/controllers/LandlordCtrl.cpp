#include "LandlordCtrl.h"
#include <fstream>
#include <algorithm>

static std::string lower(std::string s){
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
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

    Json::Value results(Json::arrayValue);
    const auto &arr = db["landlords"];
    for(const auto &ll : arr){
        std::string name = ll["name"].asString();
        // Check if input (query) is empty or the output when looking for the landlord name is NOT empty
        if(query.empty() || lower(name).find(query) != std::string::npos){
            // Searching for all names that include the query. If we input John, all entries in the json with "john" inside the landlord name will be appended to results
            results.append(ll);
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
