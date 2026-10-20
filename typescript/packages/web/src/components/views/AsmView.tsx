import { useMemo, useRef } from 'react';
import { asmInstructionCount } from '../../lib/compiler-types';
import { highlightAsmLine } from '../../lib/asm-highlight';
import { useProvenanceScroll } from '../../lib/useProvenanceScroll';
import { PanelLabel } from '../primitives';

export function AsmView({
  asm,
  metrics,
  sourceMap = [],
  hoverLine = null,
  onHoverAsm = () => {},
}: {
  asm: string;
  metrics: { compileTimeMs: number };
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
        <p className="text-[11px] text-text3">
          Abre Simulación para ejecutar el programa y explorar los registros y la pila.
        </p>
      </div>

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

