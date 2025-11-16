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
    int admin = 0;
    for(const auto &u : arr) {
        if(u["email"].asString() == email) {
            name = u.get("name","").asString();
            admin = u.get("admin", 0).asInt();
            break;
        }
    }

    // Fallback if not found (shouldn't happen right after login/signup)
    if(name.empty()) name = "User";

    Json::Value me(Json::objectValue);
    me["email"] = email;
    me["name"] = name;
    me["admin"] = admin;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(me);
    cb(resp);
}
