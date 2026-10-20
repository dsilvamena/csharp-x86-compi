import type { WebResult } from '../lib/api';
import { Card, Empty } from './primitives';
import { AstView } from './views/AstView';
import { AsmView } from './views/AsmView';
import { ErrorsView } from './views/ErrorsView';
import { OptimizationView } from './views/OptimizationView';
import { SimulationView } from './views/SimulationView';
import { TokensView } from './views/TokensView';

// IR and Symbols tabs were removed: the compiler CLI emits neither a
// three-address IR nor a symbol-table dump (see compiler-types.ts). The x86 tab
// shows the AT&T `.s`; the Simulación tab runs
// that same `.s` in the in-browser JavaScript simulator (registers/stack/…).
export type TabId = 'tokens' | 'ast' | 'x86' | 'sim' | 'optimization' | 'errors';

export const TABS: { id: TabId; label: string }[] = [
  { id: 'tokens', label: 'Tokens' },
  { id: 'ast', label: 'AST' },
  { id: 'x86', label: 'x86' },
  { id: 'sim', label: 'Simulación' },
  { id: 'optimization', label: 'Optimización' },
  { id: 'errors', label: 'Errores' },
];

export function Inspector({
  result,
  active,
  onTab,
  hoverLine = null,
  onHoverAsm = () => {},
  onExecLine = () => {},
}: {
  result: WebResult;
  active: TabId;
  onTab: (t: TabId) => void;
  hoverLine?: number | null;
  onHoverAsm?: (line: number | null) => void;
  /** Reporta al editor la línea C# que ejecuta el simulador (auto-follow). */
  onExecLine?: (line: number | null) => void;
}) {
  const errorCount = result.diagnostics.filter((d) => d.severity === 'error').length;

  return (
    <Card className="flex h-full flex-col">
      <div className="flex gap-0.5 overflow-x-auto border-b border-hair2 bg-panel2 px-2.5 pt-2">
        {TABS.map((t) => {
          const on = active === t.id;
          return (
            <button
              key={t.id}
              onClick={() => onTab(t.id)}
              className={`relative whitespace-nowrap rounded-t-[7px] px-[11px] py-[7px] text-[12.5px] transition-colors duration-150 ${
                on ? 'text-text' : 'text-text2 hover:text-text'
              }`}
            >
              {t.label}
              {t.id === 'errors' && errorCount > 0 && (
                <span className="ml-1.5 rounded-full bg-error/20 px-1.5 py-px font-mono text-[10px] text-error">
                  {errorCount}
                </span>
              )}
              {on && (
                <span className="absolute inset-x-2 -bottom-px h-0.5 rounded-sm bg-accent" />
              )}
            </button>
          );
        })}
      </div>

      <div key={active} className="view-in min-h-0 flex-1">
        {active === 'tokens' && <TokensView tokens={result.tokens} />}
        {active === 'ast' && <AstView ast={result.ast} />}
        {active === 'x86' &&
          (result.asm ? (
            <AsmView
              asm={result.asm}
              metrics={result.metrics}
              sourceMap={result.asmSourceMap}
              hoverLine={hoverLine}
              onHoverAsm={onHoverAsm}
            />
          ) : (
            <Empty>Sin ensamblador — el programa debe compilar sin errores primero.</Empty>
          ))}
        {active === 'sim' &&
          (result.asm ? (
            <SimulationView
              asm={result.asm}
              sourceMap={result.asmSourceMap}
              hoverLine={hoverLine}
              onHoverAsm={onHoverAsm}
              onExecLine={onExecLine}
            />
          ) : (
            <Empty>Sin ensamblador — el programa debe compilar sin errores primero.</Empty>
          ))}
        {active === 'optimization' && (
          <OptimizationView report={result.optReport} asm={result.asm} asmOpt={result.asmOpt} />
        )}
        {active === 'errors' && <ErrorsView diagnostics={result.diagnostics} />}
      </div>
    </Card>
  );
}
