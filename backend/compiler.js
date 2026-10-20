import { spawn } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = fileURLToPath(new URL('../', import.meta.url));
const MAX_SOURCE = 256 * 1024;
// Leave room below Vercel's response limit, including JSON serialization.
const MAX_OUTPUT = 3 * 1024 * 1024;

function backendError(message) {
  return {
    success: false,
    tokens: [], ast: '', asm: '', asmOpt: '', asmSourceMap: [], optReport: null,
    errors: [{ phase: 'backend', severity: 'error', message, line: 0, col: 0 }],
    metrics: { compileTimeMs: 0 },
  };
}

function runCompiler(source) {
  return new Promise((resolve) => {
    const compiler = process.env.COMPILER_BIN || (process.env.VERCEL
      ? path.join(process.cwd(), 'c++', 'build', 'vercel', 'compiler')
      : path.join(repo, 'c++', 'build', 'compiler'));
    const configuredTimeout = Number(process.env.COMPILE_TIMEOUT_MS || 15000);
    const timeout = Number.isFinite(configuredTimeout) && configuredTimeout > 0
      ? Math.min(configuredTimeout, 20000) : 15000;
    const child = spawn(compiler, ['--json'], { stdio: ['pipe', 'pipe', 'pipe'] });
    const chunks = [];
    let bytes = 0;
    let failure = null;
    const timer = setTimeout(() => {
      failure = { status: 504, message: 'Se excedió el tiempo máximo de compilación.' };
      child.kill('SIGKILL');
    }, timeout);

    child.stdout.on('data', (chunk) => {
      bytes += chunk.length;
      if (bytes > MAX_OUTPUT) {
        failure = { status: 413, message: 'El resultado de compilación excede el tamaño máximo.' };
        child.kill('SIGKILL');
      } else {
        chunks.push(chunk);
      }
    });
    // Drain stderr without retaining compiler internals or unbounded output.
    child.stderr.resume();
    child.on('error', (error) => {
      clearTimeout(timer);
      console.error('[compiler] No se pudo iniciar:', error.code);
      resolve({ status: 503, message: 'El servicio de compilación no está disponible.' });
    });
    child.on('close', (code) => {
      clearTimeout(timer);
      if (failure) return resolve(failure);
      try {
        const json = JSON.parse(Buffer.concat(chunks).toString('utf8'));
        // Exit 1 is the compiler's normal response to invalid C#.
        if (![0, 1].includes(code) || typeof json?.success !== 'boolean' || !Array.isArray(json.errors)) {
          throw new Error('invalid compiler response');
        }
        resolve({ status: 200, json });
      } catch {
        resolve({ status: 502, message: 'El compilador no devolvió una respuesta válida.' });
      }
    });
    child.stdin.on('error', () => {}); // EPIPE if the child exits before reading.
    child.stdin.end(source);
  });
}

export async function compileHandler(req, res) {
  res.setHeader('Cache-Control', 'no-store');
  if (req.method !== 'POST') {
    res.setHeader('Allow', 'POST');
    return res.status(405).json(backendError('Usa POST para compilar.'));
  }
  let body = req.body;
  // Vercel may expose application/x-csharp as a Buffer rather than text.
  if (Buffer.isBuffer(body)) body = body.toString('utf8');
  const source = typeof body === 'string' ? body : body?.source;
  if (typeof source !== 'string') {
    return res.status(400).json(backendError('Cuerpo inválido: falta "source" de tipo texto.'));
  }
  if (Buffer.byteLength(source, 'utf8') > MAX_SOURCE) {
    return res.status(413).json(backendError('El programa excede el tamaño máximo de 256 KB.'));
  }
  const result = await runCompiler(source);
  return res.status(result.status).json(result.json || backendError(result.message));
}
