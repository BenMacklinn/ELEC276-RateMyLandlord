#pragma once
#include <drogon/drogon.h>
#include <json/json.h>
#include <mutex>
#include <string>

/*  
    What is UserCtrl? 
    UserCtrl allows the frontend to maintian data on the current user. 
    Basically, when the user refreshes or comes back to the page they will still be logged in. 
*/

class UserCtrl {
public:
    explicit UserCtrl(const std::string &usersDbPath) : usersPath_(usersDbPath) {}
    void me(const drogon::HttpRequestPtr &req,
            std::function<void (const drogon::HttpResponsePtr &)> &&cb);

private:
    std::string usersPath_;
    std::mutex mu_;

    static std::string parseToken(const std::string &authHeader) {
        // same demo token format as AuthCtrl: "Bearer demo::<email>"
        if(authHeader.rfind("Bearer ", 0) != 0) return "";
        auto token = authHeader.substr(7);
        if(token.rfind("demo::", 0) != 0) return "";
        return token.substr(6); // email
    }
};
