// Client for the Node backend, which forwards to the C++ compiler (--json).
//
// The C++ binary is the single source of truth; the web app no longer compiles
// C# in-browser nor runs the program. `asm`/`asmOpt` arrive as ready-to-render
// AT&T text; `ast` is the textual tree from printAst; tokens/optReport/errors are
// structured. `metrics.compileTimeMs` is the real compile time.
import type { AsmSourceMap, Diagnostic, OptReport, Token } from './compiler-types';

export interface CompileMetrics {
  compileTimeMs: number;
}

/** A diagnostic as the backend emits it. */
interface BackendError {
  phase: string;
  severity?: string;
  message: string;
  line: number;
  col: number;
}

/** Raw JSON document returned by `compiler --json` (forwarded by the backend). */
interface RawResult {
  success: boolean;
  tokens: Token[];
  ast: string;
  asm: string;
  asmOpt: string;
  asmSourceMap?: AsmSourceMap;
  optReport: OptReport | null;
  errors: BackendError[];
  metrics: CompileMetrics;
}

/** Normalized result the views consume (errors mapped to the `Diagnostic` shape). */
export interface WebResult {
  ok: boolean;
  tokens: Token[];
  ast: string;
  asm: string;
  asmOpt: string;
  asmSourceMap: AsmSourceMap;  // por cada línea de `asm`: línea C# o null
  optReport: OptReport | null;
  diagnostics: Diagnostic[];
  metrics: CompileMetrics;
}

function toDiagnostics(errors: BackendError[]): Diagnostic[] {
  return errors.map((e) => ({
    phase: e.phase as Diagnostic['phase'],
    severity: (e.severity as Diagnostic['severity']) || 'error',
    message: e.message,
    line: e.line,
    col: e.col,
  }));
}

function adapt(raw: RawResult): WebResult {
  return {
    ok: raw.success,
    tokens: raw.tokens ?? [],
    ast: raw.ast ?? '',
    asm: raw.asm ?? '',
    asmOpt: raw.asmOpt ?? '',
    asmSourceMap: raw.asmSourceMap ?? [],
    optReport: raw.optReport ?? null,
    diagnostics: toDiagnostics(raw.errors ?? []),
    metrics: raw.metrics ?? { compileTimeMs: 0 },
  };
}

/** Compile `source` via the backend. Network/HTTP failures surface as one diagnostic. */
export async function compileViaBackend(source: string, signal?: AbortSignal): Promise<WebResult> {
  const res = await fetch('/api/compile', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ source }),
    signal,
  });
  const raw = (await res.json()) as RawResult;
  return adapt(raw);
}

// --- ejecución real vía Docker (Fase 4, on-demand) -----------------------
/** Result of POST /api/run: assemble + run the program inside gcc:13 (Docker). */
export interface RunResult {
  compiled: boolean;     // false ⇒ no compiló (no se ejecuta)
  ok: boolean;           // true ⇒ ensambló y ejecutó sin timeout
  output?: string;       // stdout real del programa
  stderr?: string;       // errores de ensamblado (si los hay)
  exitCode?: number;
  timedOut?: boolean;
  error?: string;        // fallo de Docker / timeout / red
}

/** Assemble + run `source` in Docker. Network/HTTP failures surface as `error`. */
export async function runViaBackend(source: string, signal?: AbortSignal): Promise<RunResult> {
  try {
    const res = await fetch('/api/run', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ source }),
      signal,
    });
    return (await res.json()) as RunResult;
  } catch (e) {
    return { compiled: false, ok: false, error: `no se pudo contactar al backend: ${(e as Error).message}` };
  }
}

export function emptyResult(): WebResult {
  return {
    ok: false, tokens: [], ast: '',
    asm: '', asmOpt: '', asmSourceMap: [], optReport: null,
    diagnostics: [], metrics: { compileTimeMs: 0 },
  };
}
