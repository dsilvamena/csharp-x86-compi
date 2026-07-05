import tailwindcss from '@tailwindcss/vite';
import react from '@vitejs/plugin-react';
import { defineConfig } from 'vite';

// The app is a static client: ALL compilation runs in the C++ binary behind the
// Node backend (--json). No TypeScript compiler/simulator remains in the app, so
// no workspace alias is needed.
export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: {
    // Proxy /api to the C++ backend so the SPA can stay a static client (:8787).
    proxy: {
      '/api': {
        target: process.env.BACKEND_URL || 'http://localhost:8787',
        changeOrigin: true,
      },
    },
  },
});
