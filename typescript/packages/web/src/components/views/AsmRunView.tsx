// x86-64 AT&T assembly viewer + on-demand native run. The CLI targets Linux ELF
// (printf@PLT, -no-pie), so the program is NOT executed in the browser. Pressing
// "Ejecutar" asks the backend to assemble + run it inside Docker (gcc:13) and
// shows the real stdout in "Salida". The old NASM/Intel simulator was retired.
import { useMemo, useRef } from 'react';
import { Play, Loader2 } from 'lucide-react';
import type { RunResult } from '../../lib/api';
import { asmInstructionCount } from '../../lib/compiler-types';
import { highlightAsmLine } from '../../lib/asm-highlight';
import { useProvenanceScroll } from '../../lib/useProvenanceScroll';
import { PanelLabel } from '../primitives';

export function AsmRunView({
  asm,
  metrics,
  run,
  running,
  onRun,
  sourceMap = [],
  hoverLine = null,
  onHoverAsm = () => {},
}: {
  asm: string;
  metrics: { compileTimeMs: number };
  run: RunResult | null;
  running: boolean;
  onRun: () => void;
  sourceMap?: (number | null)[];
  hoverLine?: number | null;
  onHoverAsm?: (line: number | null) => void;
}) {
  const lines = useMemo(() => asm.split('\n'), [asm]);
  const instr = useMemo(() => asmInstructionCount(asm), [asm]);

  const listRef = useRef<HTMLDivElement>(null);
  // Auto-scroll del listado x86 cuando el hover viene del editor C#.
  const { onListEnter, onListLeave } = useProvenanceScroll(listRef, sourceMap, hoverLine);

  return (
    <div className="flex h-full min-h-0 flex-col">
      <div className="border-b border-hair2 bg-bg2 px-3.5 py-2.5">
        <div className="mb-1 flex items-center justify-between">
          <PanelLabel>x86-64 · AT&amp;T (GNU as)</PanelLabel>
          <span className="font-mono text-[10.5px] text-text3">
            {instr} instr · {lines.length} líneas · compiló {metrics.compileTimeMs} ms
          </span>
        </div>
        <div className="flex items-center justify-between gap-3">
          <div className="font-mono text-[11px] text-text3">
            Ensamblar/ejecutar:{' '}
            <span className="text-text2">g++ -no-pie salida.s -o prog</span> (Linux/Docker).
          </div>
          <button
            onClick={onRun}
            disabled={running}
            title="Ensamblar y ejecutar en Docker (gcc:13)"
            className="flex shrink-0 items-center gap-1.5 rounded-[6px] bg-accent px-2.5 py-1 text-[11.5px] font-medium text-[#0A0B0D] transition-all duration-150 hover:bg-accent-hover active:scale-[0.97] disabled:cursor-default disabled:opacity-50"
          >
            {running ? <Loader2 size={13} className="animate-spin" /> : <Play size={13} />}
            {running ? 'Ejecutando…' : 'Ejecutar (Docker)'}
          </button>
        </div>
      </div>

      <RunOutput run={run} running={running} />

      <div
        ref={listRef}
        data-testid="asm-listing"
        className="scroll-thin min-h-0 flex-1 overflow-auto py-2 font-mono text-[12px] leading-[1.85]"
        onMouseEnter={onListEnter}
        onMouseLeave={() => { onListLeave(); onHoverAsm(null); }}
      >
        {lines.map((line, i) => {
          const src = sourceMap[i] ?? null;
          const active = hoverLine != null && src === hoverLine;
          return (
            <div
              key={i}
              className={`grid grid-cols-[40px_1fr] px-3 ${active ? 'prov-line' : ''}`}
              onMouseEnter={() => onHoverAsm(src)}
            >
              <span className="pr-3 text-right text-text3">{i + 1}</span>
              <span className="whitespace-pre">{highlightAsmLine(line)}</span>
            </div>
          );
        })}
      </div>
    </div>
  );
}

// The REAL native run, captured by the backend (Docker). Shows stdout, exit code,
// assemble errors, timeouts, or the "press Ejecutar" hint when idle.
function RunOutput({ run, running }: { run: RunResult | null; running: boolean }) {
  if (!running && !run) return null;

  let body: React.ReactNode;
  let tone = 'text-text';
  if (running) {
    body = <span className="text-text3">Ensamblando y ejecutando en Docker (gcc:13)…</span>;
  } else if (run!.error) {
    tone = 'text-error';
    body = run!.error;
  } else if (run!.compiled === false) {
    tone = 'text-text3';
    body = 'El programa no compila — corrige los errores antes de ejecutar.';
  } else if (run!.output) {
    body = run!.output;
  } else {
    body = <span className="text-text3">— sin salida —</span>;
  }

  return (
    <div className="border-b border-hair2 bg-bg2 px-3.5 py-2.5">
      <div className="mb-1 flex items-center justify-between">
        <PanelLabel>Salida · ejecución nativa (Docker gcc:13)</PanelLabel>
        {run && run.compiled !== false && !run.error && (
          <span className="font-mono text-[10.5px] text-text3">
            {run.timedOut ? (
              <span className="text-warning">timeout</span>
            ) : (
              <>exit{' '}
                <span className={run.exitCode === 0 ? 'text-success' : 'text-error'}>{run.exitCode}</span>
              </>
            )}
          </span>
        )}
      </div>
      <div className={`max-h-28 overflow-auto whitespace-pre-wrap rounded-[8px] border border-hair2 bg-panel px-3 py-2 font-mono text-[12.5px] ${tone}`}>
        {body}
      </div>
      {run?.stderr && (
        <div className="mt-1 max-h-20 overflow-auto whitespace-pre-wrap rounded-[8px] border border-error/30 bg-panel px-3 py-2 font-mono text-[11.5px] text-error">
          {run.stderr}
        </div>
      )}
    </div>
  );
}
