#include <drogon/drogon.h>
#include <sodium.h>

#include <filesystem>
#include <iostream>
#include <string>

#include "controllers/AuthCtrl.h"
#include "controllers/LandlordCtrl.h"
#include "controllers/ReviewCtrl.h"
#include "controllers/UserCtrl.h"
#include "controllers/AdminCtrl.h"

static std::string resolveDataPath(const std::string& relative) {
  namespace fs = std::filesystem;
  fs::path p1 = relative;
  fs::path p2 = fs::path("..") / relative;

  if (fs::exists(p1)) return p1.string();
  if (fs::exists(p2)) return p2.string();
  return relative;
}

int main() {
  // Initialize libsodium
  if (sodium_init() < 0) {
    std::cerr << "libsodium initialization failed\n";
    return 1;
  }

  // Server setup
  drogon::app().addListener("127.0.0.1", 8080);
  drogon::app().setThreadNum(1);
  drogon::app().setLogLevel(trantor::Logger::kInfo);
  drogon::app().enableSession();

  // -----------------------------
  //        CORS FIX
  // -----------------------------
  drogon::app().registerPostHandlingAdvice(
      [](const drogon::HttpRequestPtr& req,
         const drogon::HttpResponsePtr& resp) {
        auto mutableResp = std::const_pointer_cast<drogon::HttpResponse>(resp);

        // Get origin header
        std::string origin = req->getHeader("origin");

        // Allow local frontend
        if (origin == "http://localhost:5173" ||
            origin == "http://127.0.0.1:5173") {
          mutableResp->addHeader("Access-Control-Allow-Origin", origin);
        }

        mutableResp->addHeader("Access-Control-Allow-Credentials", "true");
        mutableResp->addHeader("Access-Control-Allow-Headers",
                               "Authorization, Content-Type");
        mutableResp->addHeader("Access-Control-Allow-Methods",
                               "GET,POST,OPTIONS");
      });

  // Handle OPTIONS preflight for unknown paths
  drogon::app().registerHandler(
      "/api/{path}",
      [](const drogon::HttpRequestPtr& req,
         std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        if (req->method() == drogon::Options) {
          auto resp = drogon::HttpResponse::newHttpResponse();
          resp->setStatusCode(drogon::k204NoContent);

          std::string origin = req->getHeader("origin");
          if (origin == "http://localhost:5173" ||
              origin == "http://127.0.0.1:5173") {
            resp->addHeader("Access-Control-Allow-Origin", origin);
          }

          resp->addHeader("Access-Control-Allow-Credentials", "true");
          resp->addHeader("Access-Control-Allow-Headers",
                          "Authorization, Content-Type");
          resp->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");

          cb(resp);
          return;
        }

        cb(drogon::HttpResponse::newNotFoundResponse());
      },
      {drogon::Get, drogon::Post, drogon::Options});

  // -----------------------------
  // Load JSON database paths
  // -----------------------------
  const std::string usersPath = resolveDataPath("data/users.json");
  const std::string landlordsPath = resolveDataPath("data/landlords.json");
  const std::string reviewsPath = resolveDataPath("data/reviews.json");
  const std::string reportedPath = resolveDataPath("data/reported.json");

  // Controllers
  auto auth = std::make_shared<AuthCtrl>(usersPath);
  auto user = std::make_shared<UserCtrl>(usersPath);
  auto landlord = std::make_shared<LandlordCtrl>(landlordsPath, landlordRequestsPath);
  auto review = std::make_shared<ReviewCtrl>();
  review->setDbPath(reviewsPath);
  auto admin = std::make_shared<AdminCtrl>(reportedPath, usersPath, reviewsPath);

  // -----------------------------
  // Authentication routes
  // -----------------------------
  drogon::app().registerHandler(
      "/api/auth/login",
      [auth](const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        auth->login(req, std::move(cb));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/auth/signup",
      [auth](const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        auth->signup(req, std::move(cb));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/auth/request-verification",
      [auth](const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        auth->requestVerification(req, std::move(cb));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/auth/verify-code",
      [auth](const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        auth->verifyCode(req, std::move(cb));
      },
      {drogon::Post});

  drogon::app().registerHandler(
    "/api/landlords/request",
    [landlord](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
      landlord->submitRequest(req, std::move(cb));
    },
    {drogon::Post});

  // -----------------------------
  // Users
  // -----------------------------
  drogon::app().registerHandler(
      "/api/users/me",
      [user](const drogon::HttpRequestPtr& req,
             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        user->me(req, std::move(cb));
      },
      {drogon::Get});

  // -----------------------------
  // Landlords
  // -----------------------------
  drogon::app().registerHandler(
      "/api/landlords/search",
      [landlord](const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        landlord->search(req, std::move(cb));
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/landlords/stats",
      [landlord](const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        landlord->stats(req, std::move(cb));
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/landlords/leaderboard",
      [landlord](const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        landlord->leaderboard(req, std::move(cb));
      },
      {drogon::Get});

  // -----------------------------
  // Reviews
  // -----------------------------
  drogon::app().registerHandler(
      "/api/reviews/submit",
      [review](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        review->submit(req, std::move(cb));
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/reviews/landlord/{id}",
      [review](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& cb,
               const std::string& id) {
        review->getForLandlord(req, std::move(cb), id);
      },
      {drogon::Get});
  
  // -----------------------------
  // Admin Landlord Requests
  // -----------------------------
  drogon::app().registerHandler(
      "/api/admin/requests",
      [landlord](const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
          landlord->listRequests(req, std::move(cb));
      },
      { drogon::Get });

  drogon::app().registerHandler(
      "/api/admin/requests/{id}/approve",
      [landlord](const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                int id) {
          landlord->approveRequest(req, std::move(cb), id);
      },
      { drogon::Post });

  drogon::app().registerHandler(
      "/api/admin/requests/{id}/reject",
      [landlord](const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                int id) {
          landlord->rejectRequest(req, std::move(cb), id);
      },
      { drogon::Post });    

  drogon::app().registerHandler(
      "/api/reviews/report",
      [review](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        review->submitReport(req, std::move(cb));
      },
      {drogon::Post});

  // -----------------------------
  // Admin: reported reviews
  // -----------------------------
  drogon::app().registerHandler(
      "/api/admin/reported",
      [admin](const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        admin->getReported(req, std::move(cb));
      },
      {drogon::Get});

  drogon::app().registerHandler(
      "/api/admin/reported/{id}/approve",
      [admin](const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
              const std::string& id) {
        admin->approve(req, std::move(cb), id);
      },
      {drogon::Post});

  drogon::app().registerHandler(
      "/api/admin/reported/{id}/deny",
      [admin](const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
              const std::string& id) {
        admin->deny(req, std::move(cb), id);
      },
      {drogon::Post});

  // -----------------------------
  // Run server
  // -----------------------------
  drogon::app().run();
  return 0;
}