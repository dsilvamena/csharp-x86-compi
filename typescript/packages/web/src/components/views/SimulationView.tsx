// Vista "Simulación": ejecuta el ensamblador x86-64 AT&T en el navegador con el
// simulador JavaScript puro (no Docker, no binario real) y muestra el estado de
// la CPU virtual: registros, flags, pila, memoria (heap/datos), salida y la línea
// del .s en ejecución.
//
// Depurador a nivel de FUENTE (C#). Toda la ejecución se pre-graba una vez en un
// arreglo de snapshots (`history`, uno por instrucción x86); el resto del
// depurador es NAVEGACIÓN sobre ese arreglo — por eso el retroceso, la línea de
// tiempo y los saltos por sentencia son instantáneos y deterministas:
//   - Paso por instrucción x86  (comportamiento original: avanza 1 snapshot)
//   - Step Into por sentencia C# (avanza hasta que cambia la línea, vía source map)
//   - Línea de tiempo con reproducción (play/pausa, velocidad, scrubbing ←→)
//
// La ejecución NATIVA real (g++ -no-pie en Docker) sigue disponible como oráculo
// en la pestaña x86; aquí todo ocurre en el cliente para visualizar el flujo.
import { useEffect, useMemo, useRef, useState } from 'react';
import {
  Play, Pause, StepForward, RotateCcw, ArrowDownToLine,
} from 'lucide-react';
import { makeSimulator, isStepLimit, type Snapshot } from '../../lib/sim';
import { highlightAsmLine } from '../../lib/asm-highlight';
import { useProvenanceScroll } from '../../lib/useProvenanceScroll';
import { PanelLabel } from '../primitives';

const REG_ORDER = [
  'rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'rbp', 'rsp',
  'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15',
];

// Tope de snapshots pre-grabados para la línea de tiempo (memoria acotada). Los
// programas del subconjunto académico terminan en muchísimos menos pasos; los
// pesados se cortan aquí y se muestra el aviso (usar breakpoints / nativo).
const TIMELINE_CAP = 20_000;
// A partir de este nº de pasos, la línea de tiempo se vuelve poco intuitiva y se
// avisa al interactuar con ella (se recomienda usar puntos de interrupción).
const TIMELINE_WARN = 5_000;
// Cadencia base de la reproducción (ms por paso a 1×); la velocidad la divide.
const PLAY_BASE_MS = 240;

type StepMode = 'stmt' | 'instr';

export function SimulationView({
  asm,
  sourceMap = [],
  hoverLine = null,
  onHoverAsm = () => {},
  onExecLine = () => {},
}: {
  asm: string;
  sourceMap?: (number | null)[];
  hoverLine?: number | null;
  onHoverAsm?: (line: number | null) => void;
  onExecLine?: (line: number | null) => void;
}) {
  const lines = useMemo(() => asm.split('\n'), [asm]);
  // Un simulador por cada .s; solo se usa una vez para pre-grabar `history`.
  const sim = useMemo(() => makeSimulator(asm), [asm]);

  // history[i] = estado tras i instrucciones (history[0] = estado inicial). El
  // índice coincide con el nº de pasos, así los algoritmos por sentencia leen
  // csLineOf/rsp directamente de cada snapshot sin volver a ejecutar el sim.
  const [history, setHistory] = useState<Snapshot[]>([]);
  const [cursor, setCursor] = useState(0);
  const [mode, setMode] = useState<StepMode>('stmt');

  // Reproducción de la línea de tiempo.
  const [playing, setPlaying] = useState(false);
  const [speed, setSpeed] = useState(1);
  const [timelineTouched, setTimelineTouched] = useState(false);

  // Pre-graba toda la ejecución al entrar a la pestaña o cambiar el .s. El cursor
  // arranca al INICIO (paso 0) para depurar desde el principio; «Ejecutar» corre
  // hasta el final (muestra la salida) y la timeline/Reiniciar navegan.
  useEffect(() => {
    sim.reset();
    const hist: Snapshot[] = [sim.snapshot()];
    while (!sim.halted && hist.length < TIMELINE_CAP) {
      const ok = sim.step();
      hist.push(sim.snapshot());
      if (!ok) break;
    }
    setHistory(hist);
    setCursor(0);
    setPlaying(false);
    setTimelineTouched(false);
  }, [sim]);

  const lastIdx = Math.max(0, history.length - 1);
  const snap: Snapshot = history[cursor] ?? sim.snapshot();
  const atEnd = cursor >= lastIdx;
  // La ejecución se cortó por el tope de snapshots (programa demasiado pesado).
  const capped = history.length > 0 && !history[lastIdx].halted;

  // Línea C# de un snapshot (índice del source map por la línea .s de la próxima
  // instrucción). null = línea .s sin correspondencia con la fuente.
  const csLineOf = useMemo(
    () => (s: Snapshot): number | null =>
      s.currentLine == null ? null : (sourceMap[s.currentLine - 1] ?? null),
    [sourceMap],
  );

  // ---- Navegación (pura sobre `history`, devuelve un índice destino) ---------

  // Step Into: avanza hasta que la línea C# cambie (saltando líneas .s sin fuente).
  const stepInto = (from: number): number => {
    const start = csLineOf(history[from]);
    let i = from;
    while (i < lastIdx) {
      i++;
      if (history[i].halted) break;
      const cs = csLineOf(history[i]);
      if (cs !== null && cs !== start) break;
    }
    return i;
  };

  const go = (target: number) => { setPlaying(false); setCursor(target); };

  const runAll = () => go(lastIdx);
  const restart = () => { setPlaying(false); setTimelineTouched(false); setCursor(0); };
  const stepInstr = () => go(Math.min(cursor + 1, lastIdx));
  const doInto = () => go(stepInto(cursor));

  // Reporta al editor la línea C# en ejecución (auto-follow). Se limpia al salir.
  const execCs = csLineOf(snap);
  useEffect(() => {
    onExecLine(execCs);
    return () => onExecLine(null);
  }, [execCs, onExecLine]);

  // Reproducción: avanza el cursor a `speed` pasos por PLAY_BASE_MS; pausa al final.
  useEffect(() => {
    if (!playing) return;
    if (atEnd) { setPlaying(false); return; }
    const id = setInterval(() => {
      setCursor((c) => {
        if (c >= lastIdx) return c;
        return c + 1;
      });
    }, PLAY_BASE_MS / speed);
    return () => clearInterval(id);
  }, [playing, speed, atEnd, lastIdx]);

  const activeIdx = snap.currentLine != null ? snap.currentLine - 1 : -1;

  const listRef = useRef<HTMLDivElement>(null);
  // Auto-scroll del listado x86 cuando el hover viene del editor C# (igual que en
  // la pestaña «x86»): trae a la vista el bloque .s de la sentencia bajo el hover.
  const { onListEnter, onListLeave } = useProvenanceScroll(listRef, sourceMap, hoverLine);

  const showTimelineWarn = timelineTouched && lastIdx > TIMELINE_WARN;

  return (
    <div className="flex h-full min-h-0 flex-col">
      {/* Barra de control */}
      <div className="border-b border-hair2 bg-bg2 px-3.5 py-2.5">
        <div className="mb-1.5 flex items-center justify-between">
          <PanelLabel>Simulación x86-64 · CPU virtual (JS)</PanelLabel>
          <span className="font-mono text-[10.5px] text-text3">
            <span data-testid="sim-cs-line" className="mr-2 text-text2">
              C#: {execCs ?? '—'}
            </span>
            <span data-testid="sim-steps">{snap.steps}</span> / {lastIdx} pasos ·{' '}
            {snap.halted ? (
              <>
                exit{' '}
                <span className={snap.exitCode === 0 ? 'text-success' : 'text-error'}>
                  {snap.exitCode}
                </span>
              </>
            ) : (
              <span className="text-warning">en ejecución</span>
            )}
          </span>
        </div>

        {/* Fila 1: modo de paso + acciones principales */}
        <div className="flex flex-wrap items-center gap-2">
          <ModeToggle mode={mode} onMode={setMode} />
          <span className="mx-0.5 h-4 w-px bg-hair" />
          <SimButton onClick={runAll} icon={<Play size={12} />} label="Ejecutar" primary />
          {mode === 'stmt' ? (
            <SimButton onClick={doInto} icon={<ArrowDownToLine size={12} />} label="Step Into" disabled={atEnd} title="Avanza una sentencia C# (entra en la llamada)" />
          ) : (
            <SimButton onClick={stepInstr} icon={<StepForward size={12} />} label="Paso" disabled={atEnd} title="Una instrucción x86" />
          )}
          <SimButton onClick={restart} icon={<RotateCcw size={12} />} label="Reiniciar" />
          <span className="ml-auto truncate font-mono text-[11px] text-text3">
            {snap.currentInstr ? <span className="text-text2">{snap.currentInstr}</span> : '— fin —'}
          </span>
        </div>

        {/* Fila 2: línea de tiempo (reproducción + scrubbing, incl. retroceso) */}
        <div className="mt-2 flex items-center gap-2">
          <button
            onClick={() => { setTimelineTouched(true); setPlaying((p) => (atEnd ? false : !p)); }}
            disabled={lastIdx === 0}
            title={playing ? 'Pausar' : 'Reproducir'}
            className="flex h-6 w-6 shrink-0 items-center justify-center rounded-[6px] border border-hair text-text2 transition-colors hover:text-text disabled:opacity-40"
          >
            {playing ? <Pause size={12} /> : <Play size={12} />}
          </button>
          <div className="flex shrink-0 gap-0.5">
            {[1, 4, 16].map((s) => (
              <button
                key={s}
                onClick={() => setSpeed(s)}
                className={`rounded-[5px] px-1.5 py-0.5 font-mono text-[10.5px] transition-colors ${
                  speed === s ? 'bg-accent/15 text-accent' : 'text-text3 hover:text-text2'
                }`}
              >
                {s}×
              </button>
            ))}
          </div>
          <input
            type="range"
            data-testid="sim-timeline"
            className="prov-range h-3 flex-1"
            min={0}
            max={lastIdx}
            value={cursor}
            onChange={(e) => { setTimelineTouched(true); setPlaying(false); setCursor(Number(e.target.value)); }}
            aria-label="Línea de tiempo"
          />
          <span className="shrink-0 font-mono text-[10.5px] text-text3">
            {cursor} / {lastIdx}
          </span>
        </div>

        {showTimelineWarn && (
          <div className="mt-2 rounded-[8px] border border-warning/30 bg-warning/10 px-3 py-2 text-[11.5px] text-warning">
            La línea de tiempo con muchos pasos puede ser poco intuitiva; usa puntos de
            interrupción para programas largos.
          </div>
        )}

        {(capped || isStepLimit(snap)) && (
          <div className="mt-2 rounded-[8px] border border-warning/30 bg-warning/10 px-3 py-2 text-[11.5px] text-warning">
            Programa demasiado pesado para simular paso a paso (se alcanzó el tope de pasos).
            Usa puntos de interrupción, o para benchmarks la ejecución nativa:{' '}
            <span className="font-mono">Ejecutar (Docker)</span> en la pestaña x86, o corre el{' '}
            <span className="font-mono">.s</span> en Linux/WSL.
          </div>
        )}
      </div>

      <div className="grid min-h-0 flex-1 grid-cols-1 lg:grid-cols-[1fr_minmax(0,300px)]">
        {/* Listado x86 con la línea actual (ejecución) y el hover C#→x86 */}
        <div
          ref={listRef}
          className="scroll-thin min-h-0 overflow-auto border-r border-hair2 py-2 font-mono text-[12px] leading-[1.8]"
          onMouseEnter={onListEnter}
          onMouseLeave={() => { onListLeave(); onHoverAsm(null); }}
        >
          {lines.map((line, i) => {
            const src = sourceMap[i] ?? null;
            return (
              <AsmLine
                key={i}
                n={i + 1}
                line={line}
                active={i === activeIdx}
                prov={hoverLine != null && src === hoverLine}
                onEnter={() => onHoverAsm(src)}
              />
            );
          })}
        </div>

        {/* Estado de la CPU */}
        <div className="scroll-thin min-h-0 overflow-auto px-3 py-2.5">
          <Section label="Salida">
            <pre className="whitespace-pre-wrap rounded-[7px] border border-hair2 bg-panel px-2.5 py-2 font-mono text-[12px] text-text">
              {snap.output || <span className="text-text3">— sin salida —</span>}
            </pre>
          </Section>

          <Section label="Registros">
            <div data-testid="sim-regs" className="grid grid-cols-2 gap-x-3 gap-y-0.5">
              {REG_ORDER.map((r) => (
                <RegRow key={r} name={r} value={snap.regs[r]} />
              ))}
            </div>
          </Section>

          <Section label="Flags">
            <div className="flex gap-2 font-mono text-[11.5px]">
              {(['ZF', 'SF', 'OF', 'CF'] as const).map((f) => (
                <span
                  key={f}
                  className={`rounded-[5px] border px-1.5 py-px ${
                    snap.flags[f]
                      ? 'border-accent/40 bg-accent/10 text-accent'
                      : 'border-hair text-text3'
                  }`}
                >
                  {f} {snap.flags[f] ? 1 : 0}
                </span>
              ))}
            </div>
          </Section>

          <Section label={`Pila · ${snap.stack.length} celdas (rsp → cima)`}>
            <MemList cells={snap.stack} rsp={snap.regs.rsp} rbp={snap.regs.rbp} />
          </Section>

          {snap.heap.length > 0 && (
            <Section label={`Heap (malloc) · ${snap.heap.length} celdas`}>
              <MemList cells={snap.heap} />
            </Section>
          )}

          {snap.data.length > 0 && (
            <Section label="Datos (.data/.rodata)">
              <div className="space-y-0.5 font-mono text-[11px]">
                {snap.data.map((d) => (
                  <div key={d.address} className="flex justify-between gap-2">
                    <span className="truncate text-text2">{d.label}</span>
                    <span className="shrink-0 text-text3">{JSON.stringify(d.value)}</span>
                  </div>
                ))}
              </div>
            </Section>
          )}
        </div>
      </div>
    </div>
  );
}

// ---- subcomponentes ---------------------------------------------------------

function ModeToggle({ mode, onMode }: { mode: StepMode; onMode: (m: StepMode) => void }) {
  return (
    <div className="flex rounded-[6px] border border-hair p-0.5 text-[11px]">
      {(['stmt', 'instr'] as const).map((m) => (
        <button
          key={m}
          onClick={() => onMode(m)}
          className={`rounded-[4px] px-2 py-0.5 transition-colors ${
            mode === m ? 'bg-accent/15 text-accent' : 'text-text3 hover:text-text2'
          }`}
        >
          {m === 'stmt' ? 'por sentencia C#' : 'por instrucción x86'}
        </button>
      ))}
    </div>
  );
}

function SimButton({
  onClick, icon, label, primary, disabled, title,
}: {
  onClick: () => void; icon: React.ReactNode; label: string; primary?: boolean; disabled?: boolean; title?: string;
}) {
  return (
    <button
      onClick={onClick}
      disabled={disabled}
      title={title}
      className={`flex items-center gap-1.5 rounded-[6px] px-2.5 py-1 text-[11.5px] font-medium transition-all duration-150 active:scale-[0.97] disabled:cursor-default disabled:opacity-40 ${
        primary
          ? 'bg-accent text-[#0A0B0D] hover:bg-accent-hover'
          : 'border border-hair text-text2 hover:text-text'
      }`}
    >
      {icon}
      {label}
    </button>
  );
}

function AsmLine({
  n, line, active, prov, onEnter,
}: {
  n: number; line: string; active: boolean; prov?: boolean; onEnter?: () => void;
}) {
  const ref = useRef<HTMLDivElement>(null);
  useEffect(() => {
    if (active) ref.current?.scrollIntoView({ block: 'nearest' });
  }, [active]);
  // La línea en ejecución (active) manda sobre el resaltado de hover (prov).
  const bg = active ? 'bg-accent/15' : prov ? 'prov-line' : '';
  return (
    <div
      ref={ref}
      className={`grid grid-cols-[40px_1fr] px-3 ${bg}`}
      onMouseEnter={onEnter}
    >
      <span className={`pr-3 text-right ${active ? 'text-accent' : 'text-text3'}`}>{n}</span>
      <span className="whitespace-pre">{highlightAsmLine(line)}</span>
    </div>
  );
}

function Section({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <div className="mb-3">
      <PanelLabel>{label}</PanelLabel>
      {children}
    </div>
  );
}

function RegRow({ name, value }: { name: string; value: number }) {
  return (
    <div className="flex justify-between font-mono text-[11.5px]">
      <span className="text-text3">{name}</span>
      <span className="text-text2">{value}</span>
    </div>
  );
}

function MemList({ cells, rsp, rbp }: { cells: { address: number; value: number }[]; rsp?: number; rbp?: number }) {
  if (cells.length === 0) return <div className="font-mono text-[11px] text-text3">— vacío —</div>;
  return (
    <div className="space-y-0.5 font-mono text-[11px]">
      {cells.map((c) => {
        const tag = c.address === rsp ? 'rsp' : c.address === rbp ? 'rbp' : '';
        return (
          <div key={c.address} className="flex justify-between gap-2">
            <span className="text-text3">
              0x{(c.address >>> 0).toString(16)}
              {tag && <span className="ml-1 text-accent">←{tag}</span>}
            </span>
            <span className="text-text2">{c.value}</span>
          </div>
        );
      })}
    </div>
  );
}
