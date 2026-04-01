// Firebase configuration for Smart Irrigation System
// Credentials loaded from .env file (VITE_ prefix required by Vite)
import { initializeApp } from "firebase/app";
import { getDatabase, ref, onValue, set } from "firebase/database";
import { getAuth, signInWithEmailAndPassword, signInAnonymously, onAuthStateChanged } from "firebase/auth";

const firebaseConfig = {
  apiKey: import.meta.env.VITE_FIREBASE_API_KEY,
  authDomain: import.meta.env.VITE_FIREBASE_AUTH_DOMAIN,
  databaseURL: import.meta.env.VITE_FIREBASE_DATABASE_URL,
  projectId: import.meta.env.VITE_FIREBASE_PROJECT_ID,
  storageBucket: import.meta.env.VITE_FIREBASE_STORAGE_BUCKET,
  messagingSenderId: import.meta.env.VITE_FIREBASE_MESSAGING_SENDER_ID,
  appId: import.meta.env.VITE_FIREBASE_APP_ID,
};

const app = initializeApp(firebaseConfig);
const database = getDatabase(app);
const auth = getAuth(app);

export { database, ref, onValue, set, auth, signInWithEmailAndPassword, signInAnonymously, onAuthStateChanged };
