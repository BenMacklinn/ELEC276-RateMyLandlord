#include "ReviewCtrl.h"

#include <chrono>
#include <fstream>
#include <random>

void ReviewCtrl::submit(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  // Parse request body
  auto json = req->getJsonObject();
  if (!json) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(
        Json::Value(Json::objectValue));
    resp->setStatusCode(drogon::k400BadRequest);
    (*resp->getJsonObject())["error"] = "Invalid JSON body";
    callback(resp);
    return;
  }

  // Validate required fields
  if (!(*json)["landlord_id"].isString() || !(*json)["rating"].isInt() ||
      !(*json)["title"].isString() || !(*json)["review"].isString()) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(
        Json::Value(Json::objectValue));
    resp->setStatusCode(drogon::k400BadRequest);
    (*resp->getJsonObject())["error"] = "Missing required fields";
    callback(resp);
    return;
  }

  // Load existing reviews
  Json::Value db;
  {
    std::lock_guard<std::mutex> lk(mu_);
    std::ifstream f(dbPath_);
    if (f.good()) {
      f >> db;
    } else {
      db["reviews"] = Json::Value(Json::arrayValue);
    }
  }

  // Create new review object
  Json::Value review;
  review["landlord_id"] = (*json)["landlord_id"];
  review["rating"] = (*json)["rating"];
  review["title"] = (*json)["title"];
  review["review"] = (*json)["review"];

  // Generate a unique ID
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(10000, 99999);
  review["id"] = std::to_string(dis(gen));

  // Add timestamp
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  char timeStr[100];
  std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ",
                std::gmtime(&now_c));
  review["created_at"] = timeStr;

  // Add to database
  db["reviews"].append(review);

  // Save back to file
  {
    std::lock_guard<std::mutex> lk(mu_);
    std::ofstream f(dbPath_);
    f << db.toStyledString();
  }

  // Return success response
  auto resp = drogon::HttpResponse::newHttpJsonResponse(review);
  callback(resp);
}

void ReviewCtrl::getForLandlord(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    const std::string &landlordId) {
  Json::Value db;
  {
    std::lock_guard<std::mutex> lk(mu_);
    std::ifstream f(dbPath_);
    if (!f.good()) {
      auto resp = drogon::HttpResponse::newHttpJsonResponse(
          Json::Value(Json::objectValue));
      resp->setStatusCode(drogon::k500InternalServerError);
      (*resp->getJsonObject())["error"] = "Reviews database missing";
      callback(resp);
      return;
    }
    f >> db;
  }

  // Filter reviews for this landlord
  Json::Value results(Json::arrayValue);
  for (const auto &review : db["reviews"]) {
    if (review["landlord_id"].asString() == landlordId) {
      results.append(review);
    }
  }

  // Return filtered reviews
  Json::Value response(Json::objectValue);
  response["reviews"] = results;
  auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
  callback(resp);
}

void ReviewCtrl::likeReview(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb,
    const std::string &reviewId) {
  std::lock_guard<std::mutex> lk(mu_);
  std::ifstream in(dbPath_ + "/reviews.json");
  if (!in.good()) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(
        Json::Value(Json::objectValue));
    resp->setStatusCode(drogon::k500InternalServerError);
    (*resp->getJsonObject())["error"] = "reviews.json not found";
    cb(resp);
    return;
  }

  Json::Value db;
  in >> db;
  in.close();
  bool found = false;
  for (auto &r : db["reviews"]) {
    if (r["id"].asString() == reviewId) {
      if (!r.isMember("likes")) r["likes"] = 0;
      r["likes"] = r["likes"].asInt() + 1;
      found = true;
      break;
    }
  }
  // hello
  if (!found) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(
        Json::Value(Json::objectValue));
    resp->setStatusCode(drogon::k404NotFound);
    (*resp->getJsonObject())["error"] = "review not found";
    cb(resp);
    return;
  }

  std::ofstream out(dbPath_ + "/reviews.json", std::ios::trunc);
  out << db;
  out.close();

  Json::Value ok(Json::objectValue);
  ok["status"] = "liked";
  auto resp = drogon::HttpResponse::newHttpJsonResponse(ok);
  cb(resp);
}

void ReviewCtrl::getLikes(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb,
    const std::string &reviewId) {
  std::ifstream in(dbPath_ + "/reviews.json");
  if (!in.good()) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(
        Json::Value(Json::objectValue));
    resp->setStatusCode(drogon::k500InternalServerError);
    (*resp->getJsonObject())["error"] = "reviews.json not found";
    cb(resp);
    return;
  }

  Json::Value db;
  in >> db;
  in.close();
  int likes = 0;
  bool found = false;
  for (const auto &r : db["reviews"]) {
    if (r["id"].asString() == reviewId) {
      likes = r.get("likes", 0).asInt();
      found = true;
      break;
    }
  }

  if (!found) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(
        Json::Value(Json::objectValue));
    resp->setStatusCode(drogon::k404NotFound);
    (*resp->getJsonObject())["error"] = "review not found";
    cb(resp);
    return;
  }

  Json::Value result(Json::objectValue);
  result["reviewId"] = reviewId;
  result["likes"] = likes;
  auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
  cb(resp);
}
