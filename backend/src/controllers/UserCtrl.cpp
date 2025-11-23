#include "UserCtrl.h"
#include "SupabaseHelper.h"
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

    // Load user data from Supabase database
    std::string name;
    bool isAdmin = false;
    std::string err;
    if(!SupabaseHelper::getUserData(email, name, isAdmin, err)) {
        LOG_ERROR << "Failed to get user data for " << email << ": " << err;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "failed to load user data";
        cb(resp);
        return;
    }

    // Fallback if name is empty
    if(name.empty()) name = "User";

    Json::Value me(Json::objectValue);
    me["email"] = email;
    me["name"] = name;
    me["admin"] = isAdmin ? 1 : 0;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(me);
    cb(resp);
}
