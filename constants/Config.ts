// ==================== CENTRALIZED CONFIGURATION ====================
// ONE SOURCE OF TRUTH - All API Keys and Configuration
// NO HARDCODED VALUES ANYWHERE IN THE PROJECT

export const API_KEYS = {
  
  // External Search APIs
  OPENAI: process.env.EXPO_PUBLIC_OPENAI_API_KEY || '',
  GOOGLE_SEARCH: process.env.EXPO_PUBLIC_GOOGLE_SEARCH_KEY || '',
  GOOGLE_SEARCH_CX: process.env.EXPO_PUBLIC_GOOGLE_SEARCH_CX || '',
  UNSPLASH: process.env.EXPO_PUBLIC_UNSPLASH_ACCESS_KEY || '',
  PEXELS: process.env.EXPO_PUBLIC_PEXELS_API_KEY || '',
  
  // AI Supporter Key (for AI Image Generation)
  AI_SUPPORT: process.env.EXPO_PUBLIC_AI_SUPPORT_KEY || '',
  
  // Groq AI API Key (for Support Chat)
  GROQ: process.env.EXPO_PUBLIC_GROQ_API_KEY || '',
  
  // Additional API Keys
  PIXABAY: process.env.EXPO_PUBLIC_PIXABAY_KEY || '',
  FLICKR: process.env.EXPO_PUBLIC_FLICKR_KEY || '',
  GIPHY: process.env.EXPO_PUBLIC_GIPHY_KEY || '',
  BING: process.env.EXPO_PUBLIC_BING_KEY || '',
  OPENVERSE: process.env.EXPO_PUBLIC_OPENVERSE_KEY || '',
  STABLE_DIFFUSION: process.env.EXPO_PUBLIC_STABLE_DIFFUSION_KEY || '',
  
  // Supabase
  SUPABASE_URL: process.env.EXPO_PUBLIC_SUPABASE_URL,
  SUPABASE_ANON_KEY: process.env.EXPO_PUBLIC_SUPABASE_ANON_KEY,
  
  // OneSignal
  ONESIGNAL_APP_ID: process.env.EXPO_PUBLIC_ONESIGNAL_APP_ID || '',
  ONESIGNAL_REST_API_KEY: process.env.ONESIGNAL_REST_API_KEY || '',
  
  // Google Auth
  GOOGLE_WEB_CLIENT_ID: process.env.EXPO_PUBLIC_GOOGLE_WEB_CLIENT_ID || '',
  GOOGLE_ANDROID_CLIENT_ID: process.env.EXPO_PUBLIC_GOOGLE_ANDROID_CLIENT_ID || '',
  
  // Database
  MONGODB_URI: process.env.MONGODB_URI || process.env.EXPO_PUBLIC_MONGODB_URI,
  
  // KRONOP BACKEND URL - SINGLE SOURCE OF TRUTH
  KRONOP_API_URL: process.env.EXPO_PUBLIC_API_BASE_URL || process.env.EXPO_PUBLIC_API_URL || 'https://kronop-9gju.onrender.com',
};


export default {
  API_KEYS,
};
