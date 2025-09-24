#include "UserCtrl.h"
#include <fstream>

void UserCtrl::me(const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
    auto authHeader = req->getHeader("authorization");
    auto email = parseToken(authHeader);
    if(email.empty()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k401Unauthorized);
        (*resp->getJsonObject())["error"] = "missing/invalid token";
        cb(resp);
        return;
    }

    // load users.json and find the matching email
    Json::Value arr(Json::arrayValue);
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::ifstream f(usersPath_);
        if(f.good()) {
            f >> arr;
        }
    }

    std::string name = "";
    for(const auto &u : arr) {
        if(u["email"].asString() == email) {
            name = u.get("name","").asString();
            break;
        }
    }

    // Fallback if not found (shouldn't happen right after login/signup)
    if(name.empty()) name = "User";

    Json::Value me(Json::objectValue);
    me["email"] = email;
    me["name"] = name;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(me);
    cb(resp);
}
