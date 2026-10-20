import { useEffect, useMemo, useRef, useState } from 'react';
import { CodeEditor } from './components/CodeEditor';
import { Hero } from './components/Hero';
import { Inspector, type TabId } from './components/Inspector';
import { Nav } from './components/Nav';
import { OptStrip } from './components/OptStrip';
import { Card, CardHeader } from './components/primitives';
import { StatusConsole } from './components/StatusConsole';
import { consoleState } from './lib/console-state';
import { compileViaBackend, emptyResult, type WebResult } from './lib/api';
import { DEFAULT_SAMPLE, SAMPLES } from './samples';

export default function App() {
  const [source, setSource] = useState(DEFAULT_SAMPLE.source);
  const [debounced, setDebounced] = useState(source);
  const [tab, setTab] = useState<TabId>('x86');

  const [result, setResult] = useState<WebResult>(emptyResult);
  const [busy, setBusy] = useState(false);
  const [backendDown, setBackendDown] = useState(false);

  // Provenance hover: the C# line under the cursor (from the editor or an asm
  // view). The editor decorates it and the asm views light up the lines that
  // line produced, via result.asmSourceMap. Bidireccional.
  const [hoverLine, setHoverLine] = useState<number | null>(null);

  // Depurador (pestaña Simulación): execLine es la línea C# que ejecuta ahora el
  // simulador; el editor la resalta y la trae a la vista (auto-follow).
  const [execLine, setExecLine] = useState<number | null>(null);

  // Debounced recompile: editing stays smooth, the request fires ~180ms after the
  // last keystroke. Compilation runs in the C++ binary behind the backend.
  useEffect(() => {
    const id = setTimeout(() => setDebounced(source), 180);
    return () => clearTimeout(id);
  }, [source]);

  // Send the source to the backend whenever it settles; stale requests are aborted.
  useEffect(() => {
    const ctrl = new AbortController();
    setBusy(true);
    setBackendDown(false);
    compileViaBackend(debounced, ctrl.signal)
      .then((r) => { setResult(r); setBackendDown(false); })
      .catch((e) => { if (e.name !== 'AbortError') setBackendDown(true); })
      .finally(() => { if (!ctrl.signal.aborted) setBusy(false); });
    return () => ctrl.abort();
  }, [debounced]);

  const status = useMemo(() => consoleState(debounced, result), [debounced, result]);

  // Primary "Ejecutar": run the generated AT&T in the in-browser JS simulator
  // (Simulación tab); the debugger opens paused at the first instruction.
  const onRun = () => setTab('sim');
  const onOptimize = () => setTab('optimization');

  const pickSample = (id: string) => {
    const s = SAMPLES.find((x) => x.id === id);
    if (s) setSource(s.source);
  };

  return (
    <div className="min-h-screen">
      <Nav onRun={onRun} onOptimize={onOptimize} />
      <Hero />

      <div className="mx-auto max-w-[1320px] px-7">
        <div className="pt-5">
          {backendDown && (
            <div className="mb-2 rounded-[8px] border border-error/30 bg-error/10 px-3 py-2 text-[12.5px] text-error">
              El servicio de compilación no está disponible en este momento. Inténtalo de nuevo en unos instantes.
            </div>
          )}
          <StatusConsole state={status} busy={busy} />
        </div>

        <div className="grid gap-[18px] py-[18px] lg:h-[640px] lg:grid-rows-1 lg:grid-cols-[0.86fr_1.14fr]">
          {/* editor */}
          <Card className="flex h-[420px] flex-col lg:h-full">
            <CardHeader
              title={<span>main.cs</span>}
              tag="C#"
              right={<SampleSelect onPick={pickSample} />}
            />
            <div className="min-h-0 flex-1">
              <CodeEditor
                value={source}
                onChange={setSource}
                hoverLine={hoverLine}
                onHoverLine={setHoverLine}
                execLine={execLine}
              />
            </div>
          </Card>

          {/* inspector */}
          <div className="h-[560px] lg:h-full">
            <Inspector
              result={result}
              active={tab}
              onTab={setTab}
              hoverLine={hoverLine}
              onHoverAsm={setHoverLine}
              onExecLine={setExecLine}
            />
          </div>
        </div>

        {result.optReport && (
          <OptStrip report={result.optReport} asm={result.asm} asmOpt={result.asmOpt} />
        )}
      </div>
    </div>
  );
}

function SampleSelect({ onPick }: { onPick: (id: string) => void }) {
  const ref = useRef<HTMLSelectElement>(null);
  return (
    <div className="relative">
      <select
        ref={ref}
        onChange={(e) => {
          onPick(e.target.value);
          e.target.selectedIndex = 0;
        }}
        className="cursor-pointer rounded-[6px] border border-hair bg-panel px-2 py-1 text-[11.5px] text-text2 outline-none transition-colors hover:text-text"
        defaultValue=""
      >
        <option value="" disabled>
          Examples…
        </option>
        {SAMPLES.map((s) => (
          <option key={s.id} value={s.id}>
            {s.label}
          </option>
        ))}
      </select>
    </div>
  );
}
