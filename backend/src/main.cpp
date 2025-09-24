#include <drogon/drogon.h>
#include <filesystem>
#include <string>

static std::string resolveDataPath(const std::string &relative) {
    namespace fs = std::filesystem;
    // try from current working dir (e.g., backend/) and from build/ (../data)
    fs::path p1 = relative;
    fs::path p2 = fs::path("..") / relative;
    if (fs::exists(p1)) return p1.string();
    if (fs::exists(p2)) return p2.string();
    // last resort: return the original (controller will report missing)
    return relative;
}

int main(){
    drogon::app().addListener("127.0.0.1", 8080);
    drogon::app().setThreadNum(1);
    drogon::app().setLogLevel(trantor::Logger::kInfo);
    drogon::app().enableSession();
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &,
                                       const drogon::HttpResponsePtr &resp) {
            auto mutableResp = std::const_pointer_cast<drogon::HttpResponse>(resp);
            mutableResp->addHeader("Access-Control-Allow-Origin", "http://127.0.0.1:5173");
            mutableResp->addHeader("Access-Control-Allow-Credentials", "true");
            mutableResp->addHeader("Access-Control-Allow-Headers", "Authorization, Content-Type");
            mutableResp->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
        });


    drogon::app().registerHandler("/api/{path}",
        [](const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
            if (req->method() == drogon::Options) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k204NoContent);
                resp->addHeader("Access-Control-Allow-Origin", "http://127.0.0.1:5173");
                resp->addHeader("Access-Control-Allow-Credentials", "true");
                resp->addHeader("Access-Control-Allow-Headers", "Authorization, Content-Type");
                resp->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
                cb(resp);
                return;
            }
            cb(drogon::HttpResponse::newNotFoundResponse());
        },
        {drogon::Get, drogon::Post, drogon::Options});

    // Handling paths to database
    const std::string usersPath = resolveDataPath("data/users.json");
    const std::string landlordsPath = resolveDataPath("data/landlords.json");

    // Handling authentication, user and landlord paths
    auto auth = std::make_shared<AuthCtrl>(usersPath);
    auto user = std::make_shared<UserCtrl>(usersPath);
    auto landlord = std::make_shared<LandlordCtrl>(landlordsPath);

    drogon::app().registerHandler("/api/auth/login",
        [auth](const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
            auth->login(req, std::move(cb));
        }, {drogon::Post});

    drogon::app().registerHandler("/api/auth/signup",
        [auth](const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
            auth->signup(req, std::move(cb));
        }, {drogon::Post});

    drogon::app().registerHandler("/api/users/me",
        [user](const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
            user->me(req, std::move(cb));
        }, {drogon::Get});

    drogon::app().registerHandler("/api/landlords/search",
        [landlord](const drogon::HttpRequestPtr &req, std::function<void (const drogon::HttpResponsePtr &)> &&cb) {
            landlord->search(req, std::move(cb));
        }, {drogon::Get});



    drogon::app().run();
    return 0;

    

    
}