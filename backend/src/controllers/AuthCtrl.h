#pragma once
#include <drogon/drogon.h>
#include <json/json.h>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include <chrono>

/*  
    What is AuthCtrl? 
    AuthCtrl's job is to handle user login and sign-ups:
        Logins: If user clicks the login button, the frontend will send a HttpRequest "/api/auth/login" 
                and the AuthCtrl::login method will promptly check if the user entered anything or if the current 
                credintials are valid. The method with then send back the email and name back to the frontend 
                in a json object.
        Signups: If the user clicks the signup button, the front end will send a HttpRequest "/api/auth/signup"
                and AuthCtrl::signup method will check if the entered credentials are avaialble. If they are, 
                We add it to the data/users.json file and end back the email and name back to the frontend 
                in a json object.
*/


class AuthCtrl {
public:
    explicit AuthCtrl(const std::string &dbPath) : dbPath_(dbPath) { loadDb(); }

    void login(const drogon::HttpRequestPtr &req,
               std::function<void (const drogon::HttpResponsePtr &)> &&cb);

    void signup(const drogon::HttpRequestPtr &req,
                std::function<void (const drogon::HttpResponsePtr &)> &&cb);

    void requestVerification(const drogon::HttpRequestPtr &req,
                             std::function<void (const drogon::HttpResponsePtr &)> &&cb);

    void verifyCode(const drogon::HttpRequestPtr &req,
                    std::function<void (const drogon::HttpResponsePtr &)> &&cb);

    // Argon2id hashing and verification helpers
    static bool hashPassword(const std::string &plain, std::string &encoded);
    static bool verifyPassword(const std::string &plain, const std::string &encoded);
    static bool isArgon2idEncoded(const std::string &maybe_encoded);

private:
    std::string dbPath_;

    struct User {
        std::string email;
        std::string password_plain; // DEMO ONLY, REMOVE IN PRODUCTION
        std::string password_hashed; // $argon2id$ PHC string format
        std::string name;
        int admin{0};
    };

    struct PendingVerification {
        std::string code;
        std::chrono::steady_clock::time_point expiresAt;
        bool verified{false};
    };
    std::unordered_map<std::string, User> users_;
    std::unordered_map<std::string, PendingVerification> pendingVerifications_;
    std::mutex mu_;

    static std::string makeToken(const std::string &email);
    static std::string parseToken(const std::string &authHeader);

    void loadDb();
};
