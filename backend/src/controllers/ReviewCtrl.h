#pragma once
#include <drogon/HttpController.h>
#include <mutex>

class ReviewCtrl : public drogon::HttpController<ReviewCtrl> {
public:
    ReviewCtrl() = default;  // Default constructor required by Drogon
    void setDbPath(const std::string &dbPath) { dbPath_ = dbPath; }

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReviewCtrl::submit, "/api/reviews/submit", drogon::Post);
    ADD_METHOD_TO(ReviewCtrl::submitReport, "/api/reviews/report", drogon::Post);
    ADD_METHOD_TO(ReviewCtrl::getForLandlord, "/api/reviews/landlord/{id}", drogon::Get);
    METHOD_LIST_END

    void submit(const drogon::HttpRequestPtr &req,
                std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    void submitReport(const drogon::HttpRequestPtr &req,
                      std::function<void (const drogon::HttpResponsePtr &)> &&callback);
    
    void getForLandlord(const drogon::HttpRequestPtr &req,
                        std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                        const std::string &landlordId);

private:
    std::string dbPath_;
    std::mutex mu_;
};