# Smart Irrigation Mini — Web App

A real-time dashboard for monitoring the Smart Irrigation system, built with React + Vite and Firebase Realtime Database.

## Getting Started

```bash
npm install
npm run dev
```

## Environment Variables

Create a `.env` file in the `webapp/` directory with the following variables.  
When deploying to **Vercel**, add these in **Project Settings → Environment Variables**:

| Variable | Description |
|---|---|
| `VITE_FIREBASE_API_KEY` | Firebase API key |
| `VITE_FIREBASE_AUTH_DOMAIN` | Firebase Auth domain (e.g. `your-app.firebaseapp.com`) |
| `VITE_FIREBASE_DATABASE_URL` | Realtime Database URL |
| `VITE_FIREBASE_PROJECT_ID` | Firebase project ID |
| `VITE_FIREBASE_STORAGE_BUCKET` | Firebase storage bucket |
| `VITE_FIREBASE_MESSAGING_SENDER_ID` | Messaging sender ID |
| `VITE_FIREBASE_APP_ID` | Firebase app ID |

> [!CAUTION]
> Never commit your `.env` file. All values above are already excluded via `.gitignore`.

## Vercel Deployment

1. Import the GitHub repo on [vercel.com](https://vercel.com)
2. Set **Root Directory** → `webapp`
3. Set **Framework** → `Vite`
4. Add all environment variables from the table above
5. Deploy 🚀
