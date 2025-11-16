#include "AuthCtrl.h"
#include <sstream>
#include <random>
#include <chrono>
#include <iomanip>
#include <curl/curl.h>
#include <cstdlib>
#include <mutex>
#include <cstring>
#include <sodium.h>

// Creating unnamed namespace to prevent varaible/functional collisons
namespace {

    // The trim function "trims" whitespace. It returns empty string if there is only whitespace. Used later for checks, and email trimming
    std::string trim(const std::string &input) {
        const auto start = input.find_first_not_of(" \r\n\t");
        if(start == std::string::npos) return "";
        const auto end = input.find_last_not_of(" \r\n\t");
        return input.substr(start, end - start + 1);
    }
    
    // Initializing libsodium for Argon2id hashing
    const int _sodium_ok = sodium_init();

    // Argon2id password hashing and verification helpers
        static bool hashPasswordArgon2id(const std::string &plain, std::string &encoded) {
        char out[crypto_pwhash_STRBYTES];
        if (crypto_pwhash_str(
                out,
                plain.c_str(), plain.size(),
                crypto_pwhash_OPSLIMIT_INTERACTIVE,
                crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
            return false;
        }
        encoded.assign(out);
        return true;
    }

    // Verifies a password against an Argon2id hash
    static bool verifyPasswordArgon2id(const std::string &plain, const std::string &encoded) {
        return crypto_pwhash_str_verify(encoded.c_str(), plain.c_str(), plain.size()) == 0;
    }

    // Checks if a string looks like an Argon2id hash
    static bool looksLikeArgon2id(const std::string &maybe) {
        return maybe.rfind("$argon2id$", 0) == 0;
    }


    // This is the random code generator for the verification code: 
    std::string generateVerificationCode() {
        static thread_local std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<int> dist(0, 999999);
        const int value = dist(rng);
        std::ostringstream oss;
        oss << std::setw(6) << std::setfill('0') << value;
        return oss.str();
    }

    /*  Using the Chrono lib, we are counting 10 minutes after the verification is sent to target email.
        Later, once this timer expires, the code will no-longer work */ 
    constexpr std::chrono::minutes kVerificationLifetime{10};


    /*  This bool function is for setting up the email transporation using Curl*/
    bool ensureCurlInit(std::string &err) {
        static std::once_flag initFlag;
        static CURLcode initCode = CURLE_OK;
        static bool initialized = false;
        static std::string initError;
        static bool cleanupRegistered = false;
        std::call_once(initFlag, [&]() {
            initCode = curl_global_init(CURL_GLOBAL_DEFAULT);
            if(initCode == CURLE_OK) {
                initialized = true;
                if(!cleanupRegistered) {
                    std::atexit([]() {
                        curl_global_cleanup();
                    });
                    cleanupRegistered = true;
                }
            } else {
                initError = curl_easy_strerror(initCode);
            }
        });
        if(!initialized) {
            err = initError.empty() ? "failed to initialize mail transport" : initError;
        }
        return initialized;
    }

    /*This is the email payload we send to users. Its a helper struct that points to the payload data*/
    struct CurlPayload {
        const std::string *data{nullptr};
        size_t offset{0};
    };

    // payloadSource copies the next chunk from CurlPayload into curl’s buffer until the payload is exhausted.
    size_t payloadSource(char *buffer, size_t size, size_t nitems, void *userdata) {
        if(!buffer || !userdata) return 0;
        auto *payload = static_cast<CurlPayload*>(userdata);

        if(!payload->data) return 0;
        const size_t bufferSize = size * nitems;

        if(bufferSize == 0) return 0;
        const size_t remaining = payload->data->size() - payload->offset;

        if(remaining == 0) return 0;
        const size_t toCopy = remaining < bufferSize ? remaining : bufferSize;

        std::memcpy(buffer, payload->data->data() + payload->offset, toCopy);
        payload->offset += toCopy;
        return toCopy;
    }

    /* This is the local notification function used only in this class. It builds and sends the actual email with SMTP*/
    bool sendVerificationEmail(const std::string &recipient, const std::string &code, std::string &err) {
        
        // These point to enviroment varaibles defined in ./run_backend that define the email information we are using to send emails
        const char *username = std::getenv("SMTP_USERNAME");
        const char *password = std::getenv("SMTP_PASSWORD");
        const char *host = std::getenv("SMTP_HOST");
        const char *port = std::getenv("SMTP_PORT");
        const char *fromEnv = std::getenv("SMTP_FROM");
        const char *fromNameEnv = std::getenv("SMTP_FROM_NAME");

        // Checks that the enviroment variables exist
        if(!username || !password) {
            err = "SMTP credentials not configured";
            return false;
        }

        const std::string hostStr = host ? host : "smtp.gmail.com";
        const std::string portStr = port ? port : "587";
        const std::string fromAddress = fromEnv ? fromEnv : username;
        const std::string fromHeader = (fromNameEnv && *fromNameEnv)
            ? (std::string(fromNameEnv) + " <" + fromAddress + ">")
            : fromAddress;

        if(!ensureCurlInit(err)) {
            return false;
        }

        // init curl pointer for mailing client
        CURL *curl = curl_easy_init();
        if(!curl) {
            err = "failed to construct mail client";
            return false;
        }

        std::string url = "smtp://" + hostStr + ":" + portStr;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME, username);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, fromAddress.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        struct curl_slist *recipients = nullptr;
        recipients = curl_slist_append(recipients, recipient.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        std::ostringstream payload;
        payload << "To: " << recipient << "\r\n";
        payload << "From: " << fromHeader << "\r\n";
        payload << "Subject: Your RateMyLandlord verification code\r\n";
        payload << "Content-Type: text/plain; charset=utf-8\r\n";
        payload << "\r\n";
        payload << "Hi,\r\n\r\n";
        payload << "Your RateMyLandlord verification code is: " << code << "\r\n";
        payload << "It expires in 10 minutes.\r\n\r\n";
        payload << "If you did not request this code you can ignore this email.\r\n";

        const std::string payloadStr = payload.str();
        CurlPayload payloadData{&payloadStr, 0};

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payloadSource);
        curl_easy_setopt(curl, CURLOPT_READDATA, &payloadData);

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            err = curl_easy_strerror(res);
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);

        return res == CURLE_OK;
    }
}

void AuthCtrl::loadDb() {
    users_.clear();
    std::ifstream f(dbPath_);
    if(!f.good()) return;
    Json::Value root;
    f >> root;
    for (const auto &u : root) {
    User user;
    user.email = u["email"].asString();
    user.name  = u.get("name", "").asString();
    user.admin = u.get("admin", 0).asInt();

    // New schema (preferred for demo): password_plain + password_hashed
    if (u.isMember("password_plain")) {
        user.password_plain = u["password_plain"].asString();
    } else if (u.isMember("password")) {
        // Legacy plaintext field named "password"
        user.password_plain = u["password"].asString();
    }

    if (u.isMember("password_hashed")) {
        user.password_hashed = u["password_hashed"].asString();
    } else if (u.isMember("password_hash")) {
        // Legacy hashed field named "password_hash"
        user.password_hashed = u["password_hash"].asString();
    } else {
        user.password_hashed.clear();
    }

    users_[user.email] = std::move(user);
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

void AuthCtrl::requestVerification(const drogon::HttpRequestPtr &req,
                                   std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
    auto json = req->getJsonObject();

    // Step 0: Checking that the json exists
    if(!json || !(*json).isMember("email")) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "email is required";
        cb(resp);
        return;
    }

    // Step 0: Check if user inputed an email
    std::string email = trim((*json)["email"].asString());
    if(email.empty()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "email is required";
        cb(resp);
        return;
    }

    // Step 0: Check if email exists in json file
    {
        std::lock_guard<std::mutex> lk(mu_);
        if(users_.find(email) != users_.end()) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k409Conflict);
            (*resp->getJsonObject())["error"] = "user already exists";
            cb(resp);
            return;
        }
    }

    // Step 1: Now that we have confirmed we can proceed with the account creation, start by generating a verification code.
    const std::string code = generateVerificationCode();

    std::string mailErr;
    if(!sendVerificationEmail(email, code, mailErr)) {
        LOG_ERROR << "Failed to send verification email to " << email << ": " << mailErr;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "failed to send verification email";
        cb(resp);
        return;
    }

    // Step 2: Wait for user to enter verification code
    {
        std::lock_guard<std::mutex> lk(mu_);
        pendingVerifications_[email] = PendingVerification{
            code,
            std::chrono::steady_clock::now() + kVerificationLifetime,
            false
        };
    }

    LOG_INFO << "Verification code emailed to " << email;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
    (*resp->getJsonObject())["message"] = "verification code sent";
    cb(resp);
}






void AuthCtrl::verifyCode(const drogon::HttpRequestPtr &req,
                          std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
    auto json = req->getJsonObject();
    if(!json || !(*json).isMember("email") || !(*json).isMember("code")) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "email and code are required";
        cb(resp);
        return;
    }

    std::string email = trim((*json)["email"].asString());
    std::string code = trim((*json)["code"].asString());
    if(email.empty() || code.empty()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "email and code are required";
        cb(resp);
        return;
    }

    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = pendingVerifications_.find(email);
        if(it == pendingVerifications_.end()) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k404NotFound);
            (*resp->getJsonObject())["error"] = "verification not found";
            cb(resp);
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if(now > it->second.expiresAt) {
            pendingVerifications_.erase(it);
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k400BadRequest);
            (*resp->getJsonObject())["error"] = "verification expired";
            cb(resp);
            return;
        }

        if(it->second.code != code) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
            resp->setStatusCode(drogon::k400BadRequest);
            (*resp->getJsonObject())["error"] = "invalid verification code";
            cb(resp);
            return;
        }

        it->second.verified = true;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
    (*resp->getJsonObject())["message"] = "email verified";
    cb(resp);
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

    // HASH secure verification + legacy upgrade
    bool ok = false;

    if (it != users_.end()) {
        const std::string &stored_phc = it->second.password_hashed;

        if (!stored_phc.empty() && AuthCtrl::isArgon2idEncoded(stored_phc)) {
        // Preferred path: verify against Argon2id PHC
            ok = AuthCtrl::verifyPassword(password, stored_phc);
        } else {
            // Legacy path: allow one-time plaintext match (demo), then upgrade to hash
            if (!it->second.password_plain.empty() && it->second.password_plain == password) {
                std::string encoded;
                if (AuthCtrl::hashPassword(password, encoded)) {
                    it->second.password_hashed = encoded;

                    // Persist upgrade with BOTH fields for demo
                    Json::Value out(Json::arrayValue);
                    for (const auto &kv : users_) {
                        const auto &usr = kv.second;
                        Json::Value u(Json::objectValue);
                        u["email"] = usr.email;
                        u["name"]  = usr.name;
                        u["password_plain"]  = usr.password_plain;   // demo only
                        u["password_hashed"] = usr.password_hashed;  // real auth
                        u["password_scheme"] = AuthCtrl::isArgon2idEncoded(usr.password_hashed)
                                           ? "argon2id-phc" : "none";
                        u["admin"] = usr.admin;
                        out.append(u);
                    }
                    std::ofstream f(dbPath_, std::ios::trunc);
                    f << out;
                    f.close();
                }
                ok = true;
            }
        }
    }

    if (it == users_.end() || !ok) {
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
    payload["admin"] = it->second.admin;
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
    std::string email = trim((*json)["email"].asString());
    std::string password = (*json)["password"].asString();
    std::string name = trim((*json)["name"].asString());

    if(email.empty() || name.empty() || password.empty()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "name, email and password required";
        cb(resp);
        return;
    }

    {
    std::lock_guard<std::mutex> lk(mu_);
    auto pendingIt = pendingVerifications_.find(email);
    const auto now = std::chrono::steady_clock::now();
    if(pendingIt == pendingVerifications_.end() || !pendingIt->second.verified || now > pendingIt->second.expiresAt) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k400BadRequest);
        (*resp->getJsonObject())["error"] = "email must be verified before signup";
        cb(resp);
        return;
    }

    if(users_.find(email) != users_.end()) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k409Conflict);
        (*resp->getJsonObject())["error"] = "user already exists";
        cb(resp);
        return;
    }

    // HASH password before saving
    std::string encoded;
    if (!AuthCtrl::hashPassword(password, encoded)) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
        resp->setStatusCode(drogon::k500InternalServerError);
        (*resp->getJsonObject())["error"] = "internal error (hashing failed)";
        cb(resp);
        return;
    }

    // Store BOTH fields (demo + real auth)
    User nu;
    nu.email = email;
    nu.name  = name;
    nu.password_plain  = password;  // DEMO ONLY
    nu.password_hashed = encoded;   // Argon2id PHC for real auth
    nu.admin = 0;
    users_[email] = std::move(nu);
    pendingVerifications_.erase(pendingIt);

    // persist to users.json (array of users)
    Json::Value out(Json::arrayValue);
    for (const auto &kv : users_) {
        const auto &usr = kv.second;
        Json::Value u(Json::objectValue);
        u["email"] = usr.email;
        u["name"]  = usr.name;
        u["password_plain"]  = usr.password_plain;   // demo
        u["password_hashed"] = usr.password_hashed;  // real auth
        u["password_scheme"] = AuthCtrl::isArgon2idEncoded(usr.password_hashed)
                       ? "argon2id-phc" : "none";
        u["admin"] = usr.admin;
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
    payload["admin"] = 0;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(payload);
    cb(resp);
}

bool AuthCtrl::hashPassword(const std::string &plain, std::string &encoded) {
    return hashPasswordArgon2id(plain, encoded);
}

bool AuthCtrl::verifyPassword(const std::string &plain, const std::string &encoded) {
    if (!looksLikeArgon2id(encoded)) return false;
        return verifyPasswordArgon2id(plain, encoded);
}

bool AuthCtrl::isArgon2idEncoded(const std::string &maybe_encoded) {
    return looksLikeArgon2id(maybe_encoded);
}
