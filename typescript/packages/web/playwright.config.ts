import { defineConfig } from '@playwright/test';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
// repo/typescript/packages/web → repo
const repo = path.resolve(__dirname, '..', '..', '..');
const compilerBin = path.join(repo, 'c++', 'build', 'compiler');
const backendDir = path.join(repo, 'backend');
const webPort = Number(process.env.E2E_WEB_PORT || 5175);
const backendPort = Number(process.env.E2E_BACKEND_PORT || 8789);

// Two servers: the Node backend (spawns the C++ compiler --json) and the Vite dev
// server on dedicated ports. The e2e drives the real binary end-to-end.
export default defineConfig({
  testDir: './e2e',
  timeout: 30_000,
  fullyParallel: false,
  use: { baseURL: `http://127.0.0.1:${webPort}`, trace: 'on-first-retry' },
  webServer: [
    {
      command: `node ${path.join(backendDir, 'server.js')}`,
      cwd: backendDir,
      port: backendPort,
      reuseExistingServer: false,
      env: { COMPILER_BIN: compilerBin, PORT: String(backendPort) },
    },
    {
      command: `npm run dev -- --host 127.0.0.1 --port ${webPort} --strictPort`,
      url: `http://127.0.0.1:${webPort}`,
      reuseExistingServer: false,
      env: { BACKEND_URL: `http://127.0.0.1:${backendPort}` },
    },
  ],
});
