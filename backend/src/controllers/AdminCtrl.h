#pragma once
#include <drogon/drogon.h>
#include <json/json.h>
#include <mutex>
#include <string>

class AdminCtrl {
public:
    explicit AdminCtrl(const std::string &reportedPath, const std::string &usersPath, const std::string &reviewsPath)
        : reportedPath_(reportedPath), usersPath_(usersPath), reviewsPath_(reviewsPath) {}

    void getReported(const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb);
    void approve(const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb, const std::string &id);
    void deny(const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb, const std::string &id);

private:
    std::string reportedPath_;
    std::string usersPath_;
    std::string reviewsPath_;
    std::mutex mu_;

    static std::string parseToken(const std::string &authHeader) {
        if(authHeader.rfind("Bearer ", 0) != 0) return "";
        auto token = authHeader.substr(7);
        if(token.rfind("demo::", 0) != 0) return "";
        return token.substr(6);
    }
    bool isAdmin(const std::string &email);
};
