#pragma once
#include <string>

namespace Json {
    class Value;
}

namespace SupabaseHelper {
    // Check if user exists in Supabase database
    // Returns true on success, false on error
    // Sets exists to true if user found, false otherwise
    bool checkUserExists(const std::string &email, bool &exists, std::string &err);

    // Insert user into Supabase database
    // Returns true on success, false on error
    bool insertUser(const std::string &email,
                    const std::string &name,
                    const std::string &password_plain,
                    const std::string &password_hashed,
                    int admin,
                    std::string &err);

    // Get user admin status from Supabase database
    // Returns true on success, false on error
    // Sets isAdmin to true if user is admin, false otherwise
    bool getUserAdminStatus(const std::string &email, bool &isAdmin, std::string &err);

    // Get user data from Supabase database
    // Returns true on success, false on error
    // Fills name and admin with user data if found
    bool getUserData(const std::string &email, std::string &name, bool &isAdmin, std::string &err);

    // Get user password hash and plaintext (for login verification)
    // Returns true on success, false on error
    // Fills password_hashed and password_plain if found
    bool getUserPasswordHash(const std::string &email, std::string &password_hashed, std::string &password_plain, std::string &name, int &admin, std::string &err);

    // Insert review into Supabase database
    // Returns true on success, false on error
    bool insertReview(const std::string &id,
                     const std::string &landlord_id,
                     int rating,
                     const std::string &title,
                     const std::string &review,
                     const std::string &created_at,
                     std::string &err);

    // Get reviews for a landlord from Supabase database
    // Returns true on success, false on error
    // Fills reviewsJson with array of reviews
    bool getReviewsForLandlord(const std::string &landlord_id, Json::Value &reviewsJson, std::string &err);

    // Get all reviews from Supabase database (for computing ratings)
    // Returns true on success, false on error
    // Fills reviewsJson with array of all reviews
    bool getAllReviews(Json::Value &reviewsJson, std::string &err);

    // Get all landlords with their properties and units from Supabase
    // Returns true on success, false on error
    // Fills landlordsJson with array of landlords (with nested properties and units)
    bool getAllLandlords(Json::Value &landlordsJson, std::string &err);

    // Get landlord statistics (counts of landlords, properties, units)
    // Returns true on success, false on error
    // Fills counts with the statistics
    bool getLandlordStats(int &landlordCount, int &propertyCount, int &unitCount, std::string &err);

    // Landlord Requests functions
    bool insertLandlordRequest(int &id,
                               const std::string &landlord_name,
                               const std::string &landlord_email,
                               const std::string &landlord_phone,
                               const std::string &user_name,
                               const std::string &user_email,
                               const std::string &details,
                               const Json::Value &properties,
                               std::string &err);

    bool getAllLandlordRequests(Json::Value &requestsJson, std::string &err);

    bool deleteLandlordRequest(int id, std::string &err);

    bool insertLandlord(const std::string &landlord_id,
                        const std::string &name,
                        const std::string &contact_email,
                        const std::string &contact_phone,
                        const Json::Value &properties,
                        std::string &err);

    // Reported Reviews functions
    bool insertReportedReview(const std::string &id,
                              const std::string &review_id,
                              const std::string &title,
                              const std::string &review,
                              const std::string &reason,
                              const std::string &reported_by,
                              const std::string &created_at,
                              std::string &err);

    bool getAllReportedReviews(Json::Value &reportsJson, std::string &err);

    bool deleteReportedReview(const std::string &id, std::string &err);

    bool deleteReview(const std::string &id, std::string &err);
}
