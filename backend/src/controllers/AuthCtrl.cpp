#include "AuthCtrl.h"
#include <sstream>

void AuthCtrl::loadDb() {
    users_.clear();
    std::ifstream f(dbPath_);
    if(!f.good()) return;
    Json::Value root;
    f >> root;
    for(const auto &u : root) {
        User user{u["email"].asString(), u["password"].asString(), u.get("name","").asString()};
        users_[user.email] = user;
    }
}

std::string AuthCtrl::makeToken(const std::string &email) {
    // DEMO token (replace with JWT/session in production)
    return "demo::" + email;
}

std::string AuthCtrl::parseToken(const std::string &authHeader) {
    // Expect "Bearer demo::<email>"
    if(authHeader.rfind("Bearer ", 0) != 0) return "";
    auto token = authHeader.substr(7);
    if(token.rfind("demo::", 0) != 0) return "";
    return token.substr(6); // email
}

void AuthCtrl::login(const drogon::HttpRequestPtr &req,
                     std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
    auto json = req->getJsonObject();

    // Checking if an email and password were inputed (confirming that the frontend sent us an email and pasword inside req data)
    if(!json || !(*json).isMember("email") || !(*json).isMember("password")) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "email and password required";
        cb(resp);
        return;
    }

    // Ok, now that we have confimred that there is either a email or password. Let us check if they are valid 
    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();
    auto it = users_.find(email);
    if(it == users_.end() || it->second.password != password) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k401Unauthorized);
        (*resp->getJsonObject())["error"] = "invalid credentials";
        cb(resp);
        return;
    }

    // Now we have passed all the login checks (Is there data? and Is the data in the database?), we can now confirm the request. 
    Json::Value payload(Json::objectValue);
    payload["token"] = makeToken(email);
    payload["name"] = it->second.name;
    payload["email"] = email;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(payload);
    cb(resp);
}

void AuthCtrl::signup(const drogon::HttpRequestPtr &req,
                      std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
    auto json = req->getJsonObject();
    if(!json || !(*json).isMember("email") || !(*json).isMember("password") || !(*json).isMember("name")) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "name, email and password required";
        cb(resp);
        return;
    }
    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();
    std::string name = (*json)["name"].asString();

    {
        std::lock_guard<std::mutex> lk(mu_);
        if(users_.find(email) != users_.end()) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k409Conflict);
            (*resp->getJsonObject())["error"] = "user already exists";
            cb(resp);
            return;
        }
        users_[email] = User{email, password, name};

        // persist to users.json (array of users)
        Json::Value out(Json::arrayValue);
        for(const auto &kv : users_) {
            Json::Value u(Json::objectValue);
            u["email"] = kv.second.email;
            u["password"] = kv.second.password;
            u["name"] = kv.second.name;
            out.append(u);
        }
        std::ofstream f(dbPath_, std::ios::trunc);
        f << out;
        f.close();
    }

    Json::Value payload(Json::objectValue);
    payload["token"] = makeToken(email);
    payload["name"] = name;
    payload["email"] = email;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(payload);
    cb(resp);
}
