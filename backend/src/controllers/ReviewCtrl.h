#pragma once
#include <drogon/HttpController.h>

#include <mutex>

class ReviewCtrl : public drogon::HttpController<ReviewCtrl> {
 public:
  ReviewCtrl() = default;  // Default constructor required by Drogon
  void setDbPath(const std::string &dbPath) { dbPath_ = dbPath; }

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ReviewCtrl::submit, "/api/reviews/submit", drogon::Post);
  ADD_METHOD_TO(ReviewCtrl::getForLandlord, "/api/reviews/landlord/{id}",
                drogon::Get);
  ADD_METHOD_TO(ReviewCtrl::likeReview, "/api/reviews/{id}/like", drogon::Post);
  ADD_METHOD_TO(ReviewCtrl::getLikes, "/api/reviews/{id}/likes", drogon::Get);
  METHOD_LIST_END

  void submit(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

  void getForLandlord(
      const drogon::HttpRequestPtr &req,
      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
      const std::string &landlordId);
  void likeReview(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                  const std::string &reviewId);

  void getLikes(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                const std::string &reviewId);

 private:
  std::string dbPath_;
  std::mutex mu_;
};