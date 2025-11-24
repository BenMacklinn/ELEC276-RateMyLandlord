# Deployment Guide

This guide walks you through deploying the RateMyLandlord application to free hosting services.

## Architecture

- **Frontend**: Vercel (free tier) - React/Vite application
- **Backend**: Railway or Render (free tier) - C++ Drogon application

## Prerequisites

1. GitHub account
2. Vercel account (free tier)
3. Railway or Render account (free tier)
4. Supabase project with database set up
5. Gmail account with App Password for SMTP

## Step 1: Deploy Backend

### Option A: Railway (Recommended)

1. **Push code to GitHub**
   ```bash
   git add .
   git commit -m "Add deployment configuration"
   git push origin main
   ```

2. **Connect to Railway**
   - Go to [railway.app](https://railway.app)
   - Click "New Project"
   - Select "Deploy from GitHub repo"
   - Choose your repository
   - Railway will detect the Dockerfile automatically

3. **Configure Environment Variables**
   - Go to your service → Variables
   - Add the following:
     ```
     SUPABASE_URL=https://your-project.supabase.co
     SUPABASE_SERVICE_ROLE_KEY=your-service-role-key
     SMTP_HOST=smtp.gmail.com
     SMTP_PORT=587
     SMTP_USERNAME=your-email@gmail.com
     SMTP_PASSWORD=your-app-password
     SMTP_FROM=your-email@gmail.com
     SMTP_FROM_NAME=RateMyLandlord
     ```

4. **Deploy**
   - Railway will automatically build and deploy
   - Wait for deployment to complete
   - Copy the generated URL (e.g., `https://rml-backend.railway.app`)

### Option B: Render

1. **Push code to GitHub** (same as above)

2. **Connect to Render**
   - Go to [render.com](https://render.com)
   - Click "New" → "Web Service"
   - Connect your GitHub repository
   - Select the repository

3. **Configure Service**
   - Name: `rml-backend`
   - Environment: Docker
   - Region: Choose closest to you
   - Branch: `main`
   - Root Directory: `backend`
   - Dockerfile Path: `backend/Dockerfile`

4. **Set Environment Variables**
   - Add all variables listed in Railway section above

5. **Deploy**
   - Click "Create Web Service"
   - Render will build and deploy
   - Copy the generated URL

## Step 2: Deploy Frontend to Vercel

1. **Install Vercel CLI** (optional, can also use web interface)
   ```bash
   npm i -g vercel
   ```

2. **Deploy via CLI**
   ```bash
   cd /path/to/your/project
   vercel
   ```
   - Follow the prompts
   - When asked for project settings, use defaults
   - When asked to override settings, say "No"

3. **Or Deploy via Web Interface**
   - Go to [vercel.com](https://vercel.com)
   - Click "Add New Project"
   - Import your GitHub repository
   - Configure:
     - Framework Preset: Vite
     - Root Directory: `frontend`
     - Build Command: `npm run build`
     - Output Directory: `dist`
     - Install Command: `npm install`

4. **Set Environment Variable**
   - Go to Project Settings → Environment Variables
   - Add: `BACKEND_URL` = your backend URL from Step 1
   - Example: `https://rml-backend.railway.app`

5. **Redeploy**
   - After adding the environment variable, trigger a new deployment
   - Vercel will rebuild with the new variable

## Step 3: Verify Deployment

1. **Test Frontend**
   - Visit your Vercel URL
   - Try logging in/signing up
   - Test API calls

2. **Test Backend**
   - Check backend health: `https://your-backend-url/api/landlords/stats`
   - Should return JSON with stats

## Troubleshooting

### Backend Issues

- **Build fails**: Check that all dependencies are in Dockerfile
- **Port issues**: Ensure backend listens on `0.0.0.0:8080` (check main.cpp)
- **Environment variables**: Verify all are set correctly

### Frontend Issues

- **API calls fail**: Check that `BACKEND_URL` is set correctly in Vercel
- **CORS errors**: The proxy function should handle this, but verify backend CORS settings
- **ReviewList.jsx hardcoded URL**: This component uses `http://127.0.0.1:8080` which won't work in production. The proxy helps, but the component may need a code change for full functionality.

### Common Issues

1. **Backend URL not working**
   - Verify the URL is accessible
   - Check Railway/Render logs for errors
   - Ensure port 8080 is exposed

2. **Environment variables not loading**
   - Redeploy after adding variables
   - Check variable names match exactly
   - Verify no typos in values

## Important: Required Code Change for Backend

**The backend must be modified to listen on `0.0.0.0` instead of `127.0.0.1` for Docker deployment.**

In `backend/src/main.cpp` line 32, change:
```cpp
drogon::app().addListener("127.0.0.1", 8080);
```
to:
```cpp
drogon::app().addListener("0.0.0.0", 8080);
```

This is necessary for the container to accept external connections. Without this change, the backend will not be accessible from outside the container.

## Notes

- **Free Tier Limitations**:
  - Railway: 500 hours/month, may require credit card
  - Render: Spins down after 15 minutes of inactivity (takes ~30s to wake up)
  - Vercel: Unlimited for hobby projects

- **ReviewList.jsx Limitation**: 
  The component has a hardcoded `http://127.0.0.1:8080` URL that won't work in production. Most functionality will work via the proxy, but this specific component may need a code change for full production support.

## Support

If you encounter issues:
1. Check deployment logs in Railway/Render/Vercel
2. Verify all environment variables are set
3. Ensure backend is accessible and responding
4. Check browser console for frontend errors

