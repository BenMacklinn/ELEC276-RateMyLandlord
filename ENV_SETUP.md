# Environment Variables Setup

This file documents all required environment variables for deployment.

## Backend Environment Variables

Set these in your Railway/Render dashboard:

### Supabase Configuration
```
SUPABASE_URL=https://your-project.supabase.co
SUPABASE_SERVICE_ROLE_KEY=your-service-role-key-here
```

### SMTP Configuration (for email verification)
```
SMTP_HOST=smtp.gmail.com
SMTP_PORT=587
SMTP_USERNAME=your-email@gmail.com
SMTP_PASSWORD=your-app-password
SMTP_FROM=your-email@gmail.com
SMTP_FROM_NAME=RateMyLandlord
```

## Frontend Environment Variables (Vercel)

Set these in your Vercel dashboard:

### Backend URL
```
BACKEND_URL=https://your-backend-url.railway.app
```
**Important**: Replace `your-backend-url.railway.app` with your actual deployed backend URL.

## Setup Instructions

1. **Backend (Railway/Render)**:
   - Go to your project settings
   - Add all backend environment variables listed above
   - Redeploy after adding variables

2. **Frontend (Vercel)**:
   - Go to your project settings → Environment Variables
   - Add `BACKEND_URL` with your backend URL
   - Redeploy after adding the variable

## Notes

- The `BACKEND_URL` in Vercel must match the URL where your backend is deployed
- For Gmail SMTP, you'll need to use an "App Password" instead of your regular password
- Never commit actual environment variable values to git

