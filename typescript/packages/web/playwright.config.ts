import { defineConfig } from '@playwright/test';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
// repo/typescript/packages/web → repo
const repo = path.resolve(__dirname, '..', '..', '..');
const compilerBin = path.join(repo, 'c++', 'compiler');
const backendDir = path.join(repo, 'backend');

// Two servers: the Node backend (spawns the C++ compiler --json) and the Vite dev
// server (which proxies /api → :8787). The e2e drives the real binary end-to-end.
export default defineConfig({
  testDir: './e2e',
  timeout: 30_000,
  fullyParallel: false,
  use: { baseURL: 'http://localhost:5173', trace: 'on-first-retry' },
  webServer: [
    {
      command: `node ${path.join(backendDir, 'server.js')}`,
      cwd: backendDir,
      port: 8787,
      reuseExistingServer: !process.env.CI,
      env: { COMPILER_BIN: compilerBin, PORT: '8787' },
    },
    {
      command: 'npm run dev -- --port 5173',
      port: 5173,
      reuseExistingServer: !process.env.CI,
    },
  ],
});
