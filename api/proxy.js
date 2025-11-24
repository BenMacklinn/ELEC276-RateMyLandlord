// Vercel serverless function to proxy all API requests to the backend
// This handles both relative path requests and the hardcoded URL issue

export default async function handler(req, res) {
  const backendUrl = process.env.BACKEND_URL;
  
  if (!backendUrl) {
    return res.status(500).json({ 
      error: 'BACKEND_URL environment variable is not set. Please configure it in Vercel dashboard.' 
    });
  }

  // Handle OPTIONS preflight first
  if (req.method === 'OPTIONS') {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS, PUT, DELETE');
    res.setHeader('Access-Control-Allow-Headers', 'Authorization, Content-Type');
    return res.status(200).end();
  }

  // Get the API path from query parameter (from rewrite) or construct from URL
  const path = req.query.path || req.url.replace('/api/', '').split('?')[0];
  const fullPath = path.startsWith('/') ? path : `/${path}`;
  
  // Construct the backend URL
  let backendRequestUrl = `${backendUrl}/api${fullPath}`;
  
  // Handle query string (exclude 'path' parameter)
  const queryParams = { ...req.query };
  delete queryParams.path; // Remove the 'path' parameter used by rewrite
  const queryString = new URLSearchParams(queryParams).toString();
  const finalUrl = queryString 
    ? `${backendRequestUrl}?${queryString}` 
    : backendRequestUrl;

  try {
    // Log for debugging
    console.log('Proxy request:', {
      method: req.method,
      path: fullPath,
      finalUrl: finalUrl,
      hasBody: !!req.body,
      bodyType: typeof req.body
    });

    // Prepare request body - Vercel already parses JSON bodies
    let requestBody = null;
    if (req.method !== 'GET' && req.method !== 'HEAD') {
      if (req.body) {
        requestBody = typeof req.body === 'string' ? req.body : JSON.stringify(req.body);
      } else if (req.method === 'POST') {
        // For POST requests, ensure we send an empty object if no body
        requestBody = '{}';
      }
    }

    // Prepare headers
    const headers = {
      'Content-Type': 'application/json',
    };
    
    if (req.headers.authorization) {
      headers['Authorization'] = req.headers.authorization;
    }

    // Forward the request to the backend
    console.log('Forwarding to backend:', finalUrl, 'Method:', req.method);
    const backendResponse = await fetch(finalUrl, {
      method: req.method,
      headers: headers,
      ...(requestBody && { body: requestBody })
    });

    console.log('Backend response status:', backendResponse.status);

    // Get response data
    const contentType = backendResponse.headers.get('content-type');
    let data;
    if (contentType && contentType.includes('application/json')) {
      data = await backendResponse.json().catch(() => ({}));
    } else {
      data = await backendResponse.text().catch(() => '');
    }

    // Forward the response with proper CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS, PUT, DELETE');
    res.setHeader('Access-Control-Allow-Headers', 'Authorization, Content-Type');

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

