// Thin bridge between the React app and the C++ compiler.
//
// The C++ binary is the single source of truth: this server NEVER compiles
// anything itself. POST /api/compile spawns `c++/compiler --json`, pipes the C#
// source to its stdin, captures stdout under a timeout, validates that it is
// JSON, and forwards it verbatim. (No HTTP server in C++ by design — decision of
// the project owner.)
import { spawn } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import express from 'express';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

// The compiled binary. Override with COMPILER_BIN if it lives elsewhere.
// `cd ../c++ && make` builds it to c++/build/compiler.
const COMPILER = process.env.COMPILER_BIN || path.resolve(__dirname, '..', 'c++', 'build', 'compiler');
const TIMEOUT_MS = Number(process.env.COMPILE_TIMEOUT_MS || 15000);
const PORT = Number(process.env.PORT || 8787);
const MAX_SOURCE = 256 * 1024; // 256 KB — these are tiny academic programs

const app = express();
app.use(express.json({ limit: '1mb' }));
app.use(express.text({ type: ['text/plain', 'application/x-csharp'], limit: '1mb' }));

// Run the C++ compiler over `source`, returning its captured stdout (or an error).
function runCompiler(source) {
  return new Promise((resolve) => {
    let child;
    try {
      child = spawn(COMPILER, ['--json'], { stdio: ['pipe', 'pipe', 'pipe'] });
    } catch (e) {
      resolve({ ok: false, error: `no se pudo ejecutar el compilador: ${e.message}` });
      return;
    }
    let out = '';
    let err = '';
    let timedOut = false;
    const timer = setTimeout(() => {
      timedOut = true;
      child.kill('SIGKILL');
    }, TIMEOUT_MS);

    child.stdout.on('data', (d) => { out += d; });
    child.stderr.on('data', (d) => { err += d; });
    child.on('error', (e) => {
      clearTimeout(timer);
      resolve({ ok: false, error: `no se pudo ejecutar el compilador: ${e.message}` });
    });
    child.on('close', (code) => {
      clearTimeout(timer);
      if (timedOut) return resolve({ ok: false, error: `tiempo de compilación excedido (${TIMEOUT_MS} ms)` });
      resolve({ ok: true, code, out, err });
    });

    child.stdin.on('error', () => {}); // ignore EPIPE if the child died early
    child.stdin.end(source);
  });
}

// Shape a backend-side failure like the compiler's own --json contract so the
// frontend can render it uniformly. Mirrors the minimal contract emitted by
// `compiler --json` (tokens/ast/asm/asmOpt/optReport/errors/metrics) — no
// ir/symbols/output (the Lab10 CLI does not produce them).
function backendError(message) {
  return {
    success: false,
    tokens: [], ast: '',
    asm: '', asmOpt: '', optReport: null,
    errors: [{ phase: 'backend', severity: 'error', message, line: 0, col: 0 }],
    metrics: { compileTimeMs: 0 },
  };
}

app.post('/api/compile', async (req, res) => {
  const source = typeof req.body === 'string' ? req.body : (req.body && req.body.source) || '';
  if (typeof source !== 'string') return res.status(400).json(backendError('cuerpo inválido: falta "source"'));
  if (source.length > MAX_SOURCE) return res.status(413).json(backendError('el programa excede el tamaño máximo'));

  const r = await runCompiler(source);
  if (!r.ok) return res.status(504).json(backendError(r.error));

  let json;
  try {
    json = JSON.parse(r.out);
  } catch {
    return res.status(502).json(backendError(`el compilador no devolvió JSON válido${r.err ? `: ${r.err.slice(0, 200)}` : ''}`));
  }
  // The compiler exits 1 on compile errors but still returns a valid JSON doc;
  // forward it as-is (success:false is carried inside).
  res.json(json);
});

// --- Ejecución real vía Docker (Fase 4, opcional) --------------------------
// El .s es AT&T/Linux y no corre nativo en macOS, así que para "ejecutar" se
// ensambla + corre dentro de gcc:13 (linux/amd64). El asm se pasa por STDIN
// (g++ -x assembler -), sin ficheros temporales ni montaje de volúmenes. Esto
// NO es parte del happy path: requiere Docker corriendo y es on-demand (no se
// dispara en cada tecla, solo al pulsar "Ejecutar").
const DOCKER_IMG = process.env.RUN_IMAGE || 'gcc:13';
const RUN_SECS = Number(process.env.RUN_TIMEOUT_SECS || 5); // timeout del programa
const DOCKER_TIMEOUT_MS = Number(process.env.DOCKER_TIMEOUT_MS || 30000); // ensamblar+arranque

// Ensambla `asm` y ejecuta el binario dentro del contenedor; captura su stdout.
function dockerRunAsm(asm) {
  return new Promise((resolve) => {
    const inner = `g++ -no-pie -x assembler - -o /tmp/prog 2>/tmp/err && timeout ${RUN_SECS} /tmp/prog; ` +
      `code=$?; if [ -s /tmp/err ]; then cat /tmp/err 1>&2; fi; exit $code`;
    const args = ['run', '--rm', '-i', '--platform', 'linux/amd64', DOCKER_IMG, 'bash', '-lc', inner];
    let child;
    try {
      child = spawn('docker', args, { stdio: ['pipe', 'pipe', 'pipe'] });
    } catch (e) {
      resolve({ ok: false, error: `no se pudo ejecutar docker: ${e.message}` });
      return;
    }
    let out = '';
    let err = '';
    let timedOut = false;
    const timer = setTimeout(() => { timedOut = true; child.kill('SIGKILL'); }, DOCKER_TIMEOUT_MS);
    child.stdout.on('data', (d) => { out += d; });
    child.stderr.on('data', (d) => { err += d; });
    child.on('error', (e) => {
      clearTimeout(timer);
      resolve({ ok: false, error: `no se pudo ejecutar docker (¿está instalado y corriendo?): ${e.message}` });
    });
    child.on('close', (code) => {
      clearTimeout(timer);
      if (timedOut) return resolve({ ok: false, timedOut: true, output: out, error: `ejecución excedió ${DOCKER_TIMEOUT_MS} ms` });
      resolve({ ok: true, output: out, stderr: err, exitCode: code });
    });
    child.stdin.on('error', () => {});
    child.stdin.end(asm);
  });
}

// POST /api/run: compila (vía --json) y, si es válido, ensambla+ejecuta en Docker.
app.post('/api/run', async (req, res) => {
  const source = typeof req.body === 'string' ? req.body : (req.body && req.body.source) || '';
  if (typeof source !== 'string') return res.status(400).json({ ok: false, error: 'cuerpo inválido: falta "source"' });
  if (source.length > MAX_SOURCE) return res.status(413).json({ ok: false, error: 'el programa excede el tamaño máximo' });

  const c = await runCompiler(source);
  if (!c.ok) return res.status(504).json({ ok: false, error: c.error });
  let json;
  try { json = JSON.parse(c.out); } catch { return res.status(502).json({ ok: false, error: 'el compilador no devolvió JSON válido' }); }
  if (!json.success) {
    // No se ejecuta un programa que no compila; se devuelven sus errores.
    return res.json({ ok: false, compiled: false, errors: json.errors || [], output: '' });
  }
  const asm = json.asmOpt || json.asm || '';
  const r = await dockerRunAsm(asm);
  res.json({ compiled: true, ...r });
});

app.get('/api/health', (_req, res) => res.json({ ok: true, compiler: COMPILER }));

app.listen(PORT, () => {
  console.log(`[backend] escuchando en http://localhost:${PORT}`);
  console.log(`[backend] compilador: ${COMPILER}`);
});
