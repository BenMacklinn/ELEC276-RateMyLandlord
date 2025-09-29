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
    explicit LandlordCtrl(const std::string &dbPath) : dbPath_(dbPath) {}
    void search(const drogon::HttpRequestPtr &req,
                std::function<void (const drogon::HttpResponsePtr &)> &&cb);
    void stats(const drogon::HttpRequestPtr &req,
               std::function<void (const drogon::HttpResponsePtr &)> &&cb);
private:
    std::string dbPath_;
    std::mutex mu_;
};
