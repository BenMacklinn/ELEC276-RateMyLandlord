#include "SupabaseHelper.h"
#include <curl/curl.h>
#include <cstdlib>
#include <vector>
#include <json/json.h>
#include <sstream>
#include <mutex>
#include <atomic>
#include <chrono>
#include <map>

namespace {
    // Helper to write curl response to string
    size_t writeToString(char *buffer, size_t size, size_t nitems, void *userdata) {
        if(!buffer || !userdata) return 0;
        auto *dest = static_cast<std::string*>(userdata);
        dest->append(buffer, size * nitems);
        return size * nitems;
    }

    // Ensure curl is initialized
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
            err = initError.empty() ? "failed to initialize curl" : initError;
        }
        return initialized;
    }

    // Get Supabase configuration from environment variables
    bool getSupabaseConfig(std::string &baseUrl, std::string &serviceRoleKey) {
        const char *urlEnv = std::getenv("SUPABASE_URL");
        const char *keyEnv = std::getenv("SUPABASE_SERVICE_ROLE_KEY");
        if(!urlEnv || !keyEnv) {
            return false;
        }
        baseUrl = urlEnv;
        serviceRoleKey = keyEnv;
        if(!baseUrl.empty() && baseUrl.back() == '/') {
            baseUrl.pop_back();
        }
        return true;
    }

    // Simple cache with TTL (30 seconds)
    struct CacheEntry {
        Json::Value data;
        std::chrono::steady_clock::time_point expiresAt;
    };
    
    std::map<std::string, CacheEntry> cache_;
    std::mutex cacheMutex_;
    const int CACHE_TTL_SECONDS = 30;

    bool getCached(const std::string &key, Json::Value &data) {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto it = cache_.find(key);
        if(it != cache_.end()) {
            auto now = std::chrono::steady_clock::now();
            if(now < it->second.expiresAt) {
                data = it->second.data;
                return true;
            } else {
                cache_.erase(it);
            }
        }
        return false;
    }

    void setCached(const std::string &key, const Json::Value &data) {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto now = std::chrono::steady_clock::now();
        CacheEntry entry;
        entry.data = data;
        entry.expiresAt = now + std::chrono::seconds(CACHE_TTL_SECONDS);
        cache_[key] = entry;
    }

    void invalidateCache(const std::string &prefix = "") {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        if(prefix.empty()) {
            cache_.clear();
        } else {
            auto it = cache_.begin();
            while(it != cache_.end()) {
                if(it->first.find(prefix) == 0) {
                    it = cache_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

namespace SupabaseHelper {

bool checkUserExists(const std::string &email, bool &exists, std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false; // Fail if Supabase not configured
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/users?email=eq." + email + "&select=email";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    // Parse response - if array has items, user exists
    Json::Value responseJson;
    Json::Reader reader;
    if(reader.parse(responseBody, responseJson) && responseJson.isArray()) {
        exists = responseJson.size() > 0;
    } else {
        exists = false;
    }

    return true;
}

bool insertUser(const std::string &email,
                const std::string &name,
                const std::string &password_plain,
                const std::string &password_hashed,
                int admin,
                std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false; // Fail if Supabase not configured
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/users";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);
    headerStrings.emplace_back("Prefer: return=representation");

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    Json::Value payload(Json::objectValue);
    payload["email"] = email;
    payload["name"] = name;
    payload["password_hashed"] = password_hashed;
    payload["password_plain"] = password_plain;
    payload["admin"] = admin;
    Json::StreamWriterBuilder writer;
    const std::string body = Json::writeString(writer, payload);

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    return true;
}

bool getUserAdminStatus(const std::string &email, bool &isAdmin, std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/users?email=eq." + email + "&select=admin";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    // Parse response - check if admin field is 1
    Json::Value responseJson;
    Json::Reader reader;
    if(reader.parse(responseBody, responseJson) && responseJson.isArray() && responseJson.size() > 0) {
        const auto &user = responseJson[0];
        isAdmin = user.get("admin", 0).asInt() == 1;
    } else {
        isAdmin = false; // User not found or invalid response
    }

    return true;
}

bool getUserData(const std::string &email, std::string &name, bool &isAdmin, std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/users?email=eq." + email + "&select=name,admin";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    // Parse response
    Json::Value responseJson;
    Json::Reader reader;
    if(reader.parse(responseBody, responseJson) && responseJson.isArray() && responseJson.size() > 0) {
        const auto &user = responseJson[0];
        name = user.get("name", "").asString();
        isAdmin = user.get("admin", 0).asInt() == 1;
    } else {
        err = "user not found";
        return false;
    }

    return true;
}

bool getUserPasswordHash(const std::string &email, std::string &password_hashed, std::string &password_plain, std::string &name, int &admin, std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/users?email=eq." + email + "&select=password_hashed,password_plain,name,admin";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    // Parse response
    Json::Value responseJson;
    Json::Reader reader;
    if(reader.parse(responseBody, responseJson) && responseJson.isArray() && responseJson.size() > 0) {
        const auto &user = responseJson[0];
        password_hashed = user.get("password_hashed", "").asString();
        password_plain = user.get("password_plain", "").asString();
        name = user.get("name", "").asString();
        admin = user.get("admin", 0).asInt();
    } else {
        err = "user not found";
        return false;
    }

    return true;
}

bool insertReview(const std::string &id,
                 const std::string &landlord_id,
                 int rating,
                 const std::string &title,
                 const std::string &review,
                 const std::string &created_at,
                 std::string &err) {
    // Invalidate reviews cache when a new review is added
    invalidateCache("reviews");
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/reviews";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);
    headerStrings.emplace_back("Prefer: return=representation");

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    Json::Value payload(Json::objectValue);
    payload["id"] = id;
    payload["landlord_id"] = landlord_id;
    payload["rating"] = rating;
    payload["title"] = title;
    payload["review"] = review;
    payload["created_at"] = created_at;
    Json::StreamWriterBuilder writer;
    const std::string body = Json::writeString(writer, payload);

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    return true;
}

bool getReviewsForLandlord(const std::string &landlord_id, Json::Value &reviewsJson, std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/reviews?landlord_id=eq." + landlord_id + "&order=created_at.desc";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    // Parse response
    Json::Reader reader;
    if(!reader.parse(responseBody, reviewsJson) || !reviewsJson.isArray()) {
        err = "invalid response format from Supabase";
        return false;
    }

    return true;
}

bool getAllReviews(Json::Value &reviewsJson, std::string &err) {
    // Check cache first
    if(getCached("reviews", reviewsJson)) {
        return true;
    }

    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/reviews?select=landlord_id,rating&order=created_at.desc";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    // Parse response
    Json::Reader reader;
    if(!reader.parse(responseBody, reviewsJson) || !reviewsJson.isArray()) {
        err = "invalid response format from Supabase";
        return false;
    }

    // Cache the result
    setCached("reviews", reviewsJson);

    return true;
}

bool getAllLandlords(Json::Value &landlordsJson, std::string &err) {
    // Check cache first
    if(getCached("landlords", landlordsJson)) {
        return true;
    }

    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    // Fetch landlords
    std::string landlordsUrl = baseUrl + "/rest/v1/landlords?select=landlord_id,name,contact_email,contact_phone";
    
    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string landlordsBody;
    curl_easy_setopt(curl, CURLOPT_URL, landlordsUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &landlordsBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + landlordsBody;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    // Parse landlords
    Json::Value landlordsArray;
    Json::Reader reader;
    if(!reader.parse(landlordsBody, landlordsArray) || !landlordsArray.isArray()) {
        err = "invalid landlords response format from Supabase";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    // Fetch properties
    std::string propertiesUrl = baseUrl + "/rest/v1/properties?select=property_id,landlord_id,street,city,province,zip";
    std::string propertiesBody;
    curl_easy_setopt(curl, CURLOPT_URL, propertiesUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &propertiesBody);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + propertiesBody;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    Json::Value propertiesArray;
    if(!reader.parse(propertiesBody, propertiesArray) || !propertiesArray.isArray()) {
        err = "invalid properties response format from Supabase";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    // Fetch units
    std::string unitsUrl = baseUrl + "/rest/v1/units?select=property_id,unit_number,bedrooms,bathrooms,rent";
    std::string unitsBody;
    curl_easy_setopt(curl, CURLOPT_URL, unitsUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &unitsBody);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + unitsBody;
        return false;
    }

    Json::Value unitsArray;
    if(!reader.parse(unitsBody, unitsArray) || !unitsArray.isArray()) {
        err = "invalid units response format from Supabase";
        return false;
    }

    // Build nested structure: landlords -> properties -> units
    landlordsJson = Json::Value(Json::arrayValue);
    
    for(const auto &landlord : landlordsArray) {
        Json::Value ll(Json::objectValue);
        ll["landlord_id"] = landlord["landlord_id"];
        ll["name"] = landlord["name"];
        
        // Build contact object
        Json::Value contact(Json::objectValue);
        contact["email"] = landlord.get("contact_email", "");
        contact["phone"] = landlord.get("contact_phone", "");
        ll["contact"] = contact;
        
        // Find properties for this landlord
        Json::Value properties(Json::arrayValue);
        for(const auto &property : propertiesArray) {
            if(property["landlord_id"].asString() == landlord["landlord_id"].asString()) {
                Json::Value prop(Json::objectValue);
                prop["property_id"] = property["property_id"];
                
                // Build address object
                Json::Value address(Json::objectValue);
                address["street"] = property.get("street", "");
                address["city"] = property.get("city", "");
                address["province"] = property.get("province", "");
                address["zip"] = property.get("zip", "");
                prop["address"] = address;
                
                // Find units for this property
                Json::Value unitDetails(Json::arrayValue);
                for(const auto &unit : unitsArray) {
                    if(unit["property_id"].asString() == property["property_id"].asString()) {
                        Json::Value u(Json::objectValue);
                        u["unit_number"] = unit.get("unit_number", "");
                        u["bedrooms"] = unit.get("bedrooms", 0);
                        u["bathrooms"] = unit.get("bathrooms", 0);
                        u["rent"] = unit.get("rent", 0);
                        unitDetails.append(u);
                    }
                }
                prop["unit_details"] = unitDetails;
                properties.append(prop);
            }
        }
        ll["properties"] = properties;
        landlordsJson.append(ll);
    }

    // Cache the result
    setCached("landlords", landlordsJson);

    return true;
}

bool getLandlordStats(int &landlordCount, int &propertyCount, int &unitCount, std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    // Get landlord count
    std::string landlordsUrl = baseUrl + "/rest/v1/landlords?select=landlord_id";
    std::string landlordsBody;
    curl_easy_setopt(curl, CURLOPT_URL, landlordsUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &landlordsBody);

    CURLcode res = curl_easy_perform(curl);
    Json::Reader reader;
    Json::Value landlordsJson;
    if(res == CURLE_OK && reader.parse(landlordsBody, landlordsJson) && landlordsJson.isArray()) {
        landlordCount = landlordsJson.size();
    } else {
        landlordCount = 0;
    }

    // Get property count
    std::string propertiesUrl = baseUrl + "/rest/v1/properties?select=property_id";
    std::string propertiesBody;
    curl_easy_setopt(curl, CURLOPT_URL, propertiesUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &propertiesBody);
    res = curl_easy_perform(curl);
    
    Json::Value propertiesJson;
    if(res == CURLE_OK && reader.parse(propertiesBody, propertiesJson) && propertiesJson.isArray()) {
        propertyCount = propertiesJson.size();
    } else {
        propertyCount = 0;
    }

    // Get unit count
    std::string unitsUrl = baseUrl + "/rest/v1/units?select=unit_id";
    std::string unitsBody;
    curl_easy_setopt(curl, CURLOPT_URL, unitsUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &unitsBody);
    res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    Json::Value unitsJson;
    if(res == CURLE_OK && reader.parse(unitsBody, unitsJson) && unitsJson.isArray()) {
        unitCount = unitsJson.size();
    } else {
        unitCount = 0;
    }

    return true;
}

bool insertLandlordRequest(int &id,
                           const std::string &landlord_name,
                           const std::string &landlord_email,
                           const std::string &landlord_phone,
                           const std::string &user_name,
                           const std::string &user_email,
                           const std::string &details,
                           const Json::Value &properties,
                           std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/landlord_requests";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);
    headerStrings.emplace_back("Prefer: return=representation");

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    Json::Value payload(Json::objectValue);
    payload["landlord_name"] = landlord_name;
    payload["landlord_email"] = landlord_email;
    payload["landlord_phone"] = landlord_phone;
    payload["user_name"] = user_name;
    payload["user_email"] = user_email;
    payload["details"] = details;
    payload["properties"] = properties;
    // id and created_at will be auto-generated by database

    Json::StreamWriterBuilder writer;
    const std::string body = Json::writeString(writer, payload);

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    // Parse response to get the generated id
    Json::Value responseJson;
    Json::Reader reader;
    if(reader.parse(responseBody, responseJson) && responseJson.isArray() && responseJson.size() > 0) {
        id = responseJson[0].get("id", 0).asInt();
    } else {
        err = "invalid response format from Supabase";
        return false;
    }

    return true;
}

bool getAllLandlordRequests(Json::Value &requestsJson, std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/landlord_requests?order=created_at.desc";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    Json::Reader reader;
    if(!reader.parse(responseBody, requestsJson) || !requestsJson.isArray()) {
        err = "invalid response format from Supabase";
        return false;
    }

    return true;
}

bool deleteLandlordRequest(int id, std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/landlord_requests?id=eq." + std::to_string(id);

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    return true;
}

bool insertLandlord(const std::string &landlord_id,
                    const std::string &name,
                    const std::string &contact_email,
                    const std::string &contact_phone,
                    const Json::Value &properties,
                    std::string &err) {
    // Invalidate landlords cache when a new landlord is added
    invalidateCache("landlords");
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    // Insert landlord
    std::string landlordUrl = baseUrl + "/rest/v1/landlords";
    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);
    headerStrings.emplace_back("Prefer: return=representation");

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    Json::Value landlordPayload(Json::objectValue);
    landlordPayload["landlord_id"] = landlord_id;
    landlordPayload["name"] = name;
    landlordPayload["contact_email"] = contact_email;
    landlordPayload["contact_phone"] = contact_phone;

    Json::StreamWriterBuilder writer;
    std::string landlordBody = Json::writeString(writer, landlordPayload);

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, landlordUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, landlordBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    // Insert properties and units
    bool allPropertiesInserted = true;
    std::string propertyErrors;
    for(const auto &prop : properties) {
        std::string propertyId = prop["property_id"].asString();
        std::string street = prop["address"]["street"].asString();
        std::string city = prop["address"]["city"].asString();
        std::string province = prop["address"].get("province", prop["address"].get("state", "")).asString();
        std::string zip = prop["address"]["zip"].asString();

        // Insert property
        std::string propertyUrl = baseUrl + "/rest/v1/properties";
        Json::Value propPayload(Json::objectValue);
        propPayload["property_id"] = propertyId;
        propPayload["landlord_id"] = landlord_id;
        propPayload["street"] = street;
        propPayload["city"] = city;
        propPayload["province"] = province;
        propPayload["zip"] = zip;

        std::string propBody = Json::writeString(writer, propPayload);
        curl_easy_setopt(curl, CURLOPT_URL, propertyUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, propBody.c_str());
        responseBody.clear();
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        if(res != CURLE_OK || (httpCode < 200 || httpCode >= 300)) {
            allPropertiesInserted = false;
            propertyErrors += "Property " + propertyId + " failed; ";
            continue; // Skip units for this property if property insert failed
        }

        // Insert units
        const auto &units = prop["unit_details"];
        for(const auto &unit : units) {
            std::string unitUrl = baseUrl + "/rest/v1/units";
            Json::Value unitPayload(Json::objectValue);
            unitPayload["property_id"] = propertyId;
            unitPayload["unit_number"] = unit.get("unit_number", "").asString();
            unitPayload["bedrooms"] = unit.get("bedrooms", 0).asInt();
            unitPayload["bathrooms"] = unit.get("bathrooms", 0).asDouble();
            unitPayload["rent"] = unit.get("rent", 0).asInt();

            std::string unitBody = Json::writeString(writer, unitPayload);
            curl_easy_setopt(curl, CURLOPT_URL, unitUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, unitBody.c_str());
            responseBody.clear();
            res = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            if(res != CURLE_OK || (httpCode < 200 || httpCode >= 300)) {
                propertyErrors += "Unit in " + propertyId + " failed; ";
            }
        }
    }

    // If some properties/units failed, log warning but don't fail the whole operation
    if(!allPropertiesInserted) {
        err = "Landlord created but some properties/units failed: " + propertyErrors;
        // Don't return false - landlord was created successfully
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return true;
}

bool insertReportedReview(const std::string &id,
                          const std::string &review_id,
                          const std::string &title,
                          const std::string &review,
                          const std::string &reason,
                          const std::string &reported_by,
                          const std::string &created_at,
                          std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/reported_reviews";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);
    headerStrings.emplace_back("Prefer: return=representation");

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    Json::Value payload(Json::objectValue);
    payload["id"] = id;
    payload["review_id"] = review_id;
    payload["title"] = title;
    payload["review"] = review;
    payload["reason"] = reason;
    payload["reported_by"] = reported_by;
    payload["created_at"] = created_at;
    payload["status"] = "pending";

    Json::StreamWriterBuilder writer;
    const std::string body = Json::writeString(writer, payload);

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    return true;
}

bool getAllReportedReviews(Json::Value &reportsJson, std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/reported_reviews?order=created_at.desc";

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    Json::Reader reader;
    if(!reader.parse(responseBody, reportsJson) || !reportsJson.isArray()) {
        err = "invalid response format from Supabase";
        return false;
    }

    return true;
}

bool deleteReportedReview(const std::string &id, std::string &err) {
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/reported_reviews?id=eq." + id;

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    return true;
}

bool deleteReview(const std::string &id, std::string &err) {
    // Invalidate reviews cache when a review is deleted
    invalidateCache("reviews");
    std::string baseUrl;
    std::string serviceRoleKey;
    if(!getSupabaseConfig(baseUrl, serviceRoleKey)) {
        err = "Supabase not configured (SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY required)";
        return false;
    }

    if(!ensureCurlInit(err)) {
        return false;
    }

    std::string url = baseUrl + "/rest/v1/reviews?id=eq." + id;

    CURL *curl = curl_easy_init();
    if(!curl) {
        err = "failed to construct supabase client";
        return false;
    }

    std::vector<std::string> headerStrings;
    headerStrings.emplace_back("Content-Type: application/json");
    headerStrings.emplace_back("apikey: " + serviceRoleKey);
    headerStrings.emplace_back("Authorization: Bearer " + serviceRoleKey);

    struct curl_slist *headers = nullptr;
    for(const auto &h : headerStrings) {
        headers = curl_slist_append(headers, h.c_str());
    }

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        err = curl_easy_strerror(res);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if(httpCode < 200 || httpCode >= 300) {
        err = "supabase returned HTTP " + std::to_string(httpCode) + ": " + responseBody;
        return false;
    }

    return true;
}

}
