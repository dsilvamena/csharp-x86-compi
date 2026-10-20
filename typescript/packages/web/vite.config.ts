import tailwindcss from '@tailwindcss/vite';
import react from '@vitejs/plugin-react';
import { defineConfig } from 'vite';

// Compilation runs in the C++ backend; x86 simulation runs in the browser.
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
