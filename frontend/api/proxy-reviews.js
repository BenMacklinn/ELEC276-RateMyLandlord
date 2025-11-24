// Vercel serverless function to proxy requests from ReviewList.jsx
// This handles the hardcoded URL issue without modifying the component

export default async function handler(req, res) {
  const backendUrl = process.env.BACKEND_URL || 'http://127.0.0.1:8080';
  
  // Extract the landlord ID from the request
  const { id } = req.query;
  
  if (!id) {
    return res.status(400).json({ error: 'Landlord ID is required' });
  }

  try {
    // Forward the request to the backend
    const backendResponse = await fetch(`${backendUrl}/api/reviews/landlord/${id}`, {
      method: 'GET',
      headers: {
        'Content-Type': 'application/json',
        ...(req.headers.authorization && { 'Authorization': req.headers.authorization })
      }
    });

    const data = await backendResponse.json();

    // Forward the response with proper CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Authorization, Content-Type');

    if (!backendResponse.ok) {
      return res.status(backendResponse.status).json(data);
    }

    return res.status(200).json(data);
  } catch (error) {
    console.error('Proxy error:', error);
    return res.status(500).json({ error: 'Failed to proxy request to backend' });
  }
}

