import React, { useEffect, useState } from "react";

export default function ReviewList({ landlordId }) {
    const [reviews, setReviews] = useState([]);
    const [loading, setLoading] = useState(true);

    useEffect(() => {
        console.log("🔍 ReviewList mounted. landlordId =", landlordId);

        if (!landlordId) {
            console.log("❌ No landlordId passed into ReviewList.");
            setLoading(false);
            return;
        }

        const url = `http://127.0.0.1:8080/api/reviews/landlord/${landlordId}`;
        console.log("🌐 Fetching reviews from:", url);

        fetch(url)
            .then(res => {
                console.log("📥 Raw response:", res);
                return res.json();
            })
            .then(data => {
                console.log("✅ Parsed review data:", data);
                setReviews(data.reviews || []);
            })
            .catch(err => {
                console.error("❌ Failed to fetch reviews:", err);
            })
            .finally(() => {
                console.log("⏳ Finished fetch, stopping loading.");
                setLoading(false);
            });

    }, [landlordId]);

    // ==== STYLES ====
    const reviewBox = {
        backgroundColor: "rgba(255,255,255,0.1)",
        border: "1px solid rgba(255,255,255,0.3)",
        padding: "16px",
        borderRadius: "12px",
        marginBottom: "12px",
        backdropFilter: "blur(6px)",
        color: "#fff"
    };

    // ==== UI STATES ====
    if (loading) {
        return <div style={{ color: "#fff", marginTop: "10px" }}>Loading reviews...</div>;
    }

    if (!reviews.length) {
        return <div style={{ color: "#fff", marginTop: "10px" }}>No reviews yet.</div>;
    }

    // ==== RENDER REVIEWS ====
    return (
        <div style={{ marginTop: "10px" }}>
            {reviews.map((rev) => (
                <div key={rev.id} style={reviewBox}>
                    <h4 style={{ margin: 0 }}>
                        {rev.title} — ⭐ {rev.rating}
                    </h4>
                    <p style={{ marginTop: "6px" }}>{rev.review}</p>
                    <span style={{ fontSize: "12px", opacity: 0.7 }}>
                        {rev.created_at}
                    </span>
                </div>
            ))}
        </div>
    );
}
