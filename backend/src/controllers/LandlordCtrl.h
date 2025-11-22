#pragma once
#include <drogon/drogon.h>
#include <json/json.h>
#include <mutex>

/*  
    What is LandlordCtrl? 
    LandlordCtrl's Only Current job is to recieve a landlord request from the front end, then 
    spit out a list of landlords that match the entered name in the search bar as a json. 
*/

class LandlordCtrl {
public:
    explicit LandlordCtrl(const std::string &dbPath, const std::string &requestDbPath) : dbPath_(dbPath), requestDbPath_(requestDbPath) {}
    void search(const drogon::HttpRequestPtr &req,
                std::function<void (const drogon::HttpResponsePtr &)> &&cb);
    void stats(const drogon::HttpRequestPtr &req,
               std::function<void (const drogon::HttpResponsePtr &)> &&cb);
    void leaderboard(const drogon::HttpRequestPtr &req,
                    std::function<void (const drogon::HttpResponsePtr &)> &&cb);
    void submitRequest(const drogon::HttpRequestPtr &req,
                       std::function<void (const drogon::HttpResponsePtr &)> &&cb);
    void listRequests(const drogon::HttpRequestPtr &req,
                      std::function<void (const drogon::HttpResponsePtr &)> &&cb);
    void approveRequest(const drogon::HttpRequestPtr &req,
                        std::function<void (const drogon::HttpResponsePtr &)> &&cb,
                        int requestId);
    void rejectRequest(const drogon::HttpRequestPtr &req,
                       std::function<void (const drogon::HttpResponsePtr &)> &&cb,
                       int requestId);
     
private:
    std::string dbPath_;
    std::string requestDbPath_;
    std::mutex mu_;
};
