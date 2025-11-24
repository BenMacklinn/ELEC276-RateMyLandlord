// Vercel serverless function to proxy all API requests to the backend
// This handles both relative path requests and the hardcoded URL issue

export default async function handler(req, res) {
  const backendUrl = process.env.BACKEND_URL;
  
  if (!backendUrl) {
    return res.status(500).json({ 
      error: 'BACKEND_URL environment variable is not set. Please configure it in Vercel dashboard.' 
    });
  }

  // Get the API path from query parameter (from rewrite) or construct from URL
  const path = req.query.path || req.url.replace('/api/', '');
  const fullPath = path.startsWith('/') ? path : `/${path}`;
  
  // Construct the backend URL
  const backendRequestUrl = `${backendUrl}/api${fullPath}`;
  
  // Handle query string
  const queryString = new URLSearchParams(req.query).toString();
  const finalUrl = queryString && !path.includes('?') 
    ? `${backendRequestUrl}?${queryString}` 
    : backendRequestUrl;

  try {
    // Prepare request body
    let requestBody = null;
    if (req.method !== 'GET' && req.method !== 'HEAD' && req.method !== 'OPTIONS') {
      if (req.body) {
        requestBody = typeof req.body === 'string' ? req.body : JSON.stringify(req.body);
      }
    }

    // Forward the request to the backend
    const backendResponse = await fetch(finalUrl, {
      method: req.method,
      headers: {
        'Content-Type': 'application/json',
        ...(req.headers.authorization && { 'Authorization': req.headers.authorization }),
        ...(req.headers['content-type'] && { 'Content-Type': req.headers['content-type'] })
      },
      ...(requestBody && { body: requestBody })
    });

    const data = await backendResponse.json().catch(() => ({}));

    // Forward the response with proper CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS, PUT, DELETE');
    res.setHeader('Access-Control-Allow-Headers', 'Authorization, Content-Type');

    // Handle OPTIONS preflight
    if (req.method === 'OPTIONS') {
      return res.status(200).end();
    }

    if (!backendResponse.ok) {
      return res.status(backendResponse.status).json(data);
    }

    return res.status(200).json(data);
  } catch (error) {
    console.error('Proxy error:', error);
    return res.status(500).json({ 
      error: 'Failed to proxy request to backend',
      details: error.message 
    });
  }
}

