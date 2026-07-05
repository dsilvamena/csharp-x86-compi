// Optimization report. The optimizer has two measured passes: constant folding
// (ConstantVisitor) and peephole over the `.s`. We show those real counters plus
// the instruction count before (--no-opt) and after (--opt), and an ALIGNED
// side-by-side diff of both `.s`: removed instructions light up red on the left
// and the folded/optimized result lights up green on the right (same row), so
// it's visible what the optimizer actually did.
import { Fragment, useMemo } from 'react';
import type { OptReport } from '../../lib/compiler-types';
import { highlightAsmLine } from '../../lib/asm-highlight';
import { splitDiff, type DiffRow } from '../../lib/asmDiff';
import { Empty, PanelLabel } from '../primitives';

export function OptimizationView({
  report,
  asm,
  asmOpt,
}: {
  report: OptReport | null;
  asm: string | null;
  asmOpt: string | null;
}) {
  const rows = useMemo(
    () => (asm && asmOpt ? splitDiff(asm, asmOpt) : []),
    [asm, asmOpt],
  );
  const changed = rows.filter((r) => !r.same).length;

  if (!report) return <Empty>Sin reporte de optimización — compila un programa sin errores primero.</Empty>;

  return (
    <div className="flex h-full flex-col">
      <div className="border-b border-hair2 p-3.5">
        <div className="text-[13px] text-text2">
          <span className="font-semibold text-text">Reporte de optimización.</span> Constantes
          plegadas: {report.constantFolding} | Líneas peephole: {report.peepholeRemoved} |
          Instrucciones: {report.instructionsBefore} → {report.instructionsAfter} | Mejora:{' '}
          {report.reductionPercent}%
        </div>

        <div className="mt-3 grid grid-cols-2 gap-2 sm:grid-cols-4">
          <Stat
            k="Instr. .s"
            v={`${report.instructionsBefore} → ${report.instructionsAfter}`}
            title="Instrucciones x86 reales antes → después (excluye líneas en blanco, comentarios, directivas y etiquetas)."
          />
          <Stat k="Eliminadas" v={String(report.removed)} title="Instrucciones x86 eliminadas por la optimización." />
          <Stat
            k="Mejora"
            v={`${report.reductionPercent}%`}
            accent
            title="Reducción de instrucciones x86: eliminadas / antes × 100."
          />
          <Stat k="Const. plegadas" v={String(report.constantFolding)} title="Expresiones constantes evaluadas en compilación (ConstantVisitor)." />
        </div>

        <div className="mt-3">
          <PanelLabel>Optimizaciones</PanelLabel>
          <div className="grid grid-cols-2 gap-1.5 sm:grid-cols-2">
            <Counter label="Constant folding (ConstantVisitor)" v={report.constantFolding} />
            <Counter label="Peephole (líneas eliminadas)" v={report.peepholeRemoved} />
          </div>
        </div>
      </div>

      {asm && asmOpt && (
        <div className="flex min-h-0 flex-1 flex-col">
          {/* Cabeceras + leyenda del diff */}
          <div className="grid grid-cols-2 border-b border-hair2 bg-panel2 text-[11px] uppercase tracking-wide text-text3">
            <div className="flex items-center justify-between border-r border-hair2 px-3 py-1.5">
              <span>x86 — sin optimizar</span>
              <span className="flex items-center gap-1 font-mono normal-case tracking-normal">
                <Swatch className="bg-error/60" /> eliminado
              </span>
            </div>
            <div className="flex items-center justify-between px-3 py-1.5">
              <span>x86 — optimizado</span>
              <span className="flex items-center gap-1 font-mono normal-case tracking-normal">
                <Swatch className="bg-success/60" /> optimizado · {changed} cambios
              </span>
            </div>
          </div>

          {/* Diff alineado: una fila = par (izquierda, derecha) del grid. */}
          <div className="scroll-thin min-h-0 flex-1 overflow-auto font-mono text-[11.5px] leading-[1.85]">
            <div className="grid grid-cols-2">
              {rows.map((r, i) => (
                <Fragment key={i}>
                  <DiffCell text={r.left} no={r.leftNo} side="left" same={r.same} />
                  <DiffCell text={r.right} no={r.rightNo} side="right" same={r.same} />
                </Fragment>
              ))}
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

function DiffCell({
  text, no, side, same,
}: {
  text: string | null; no: number | null; side: 'left' | 'right'; same: boolean;
}) {
  const removed = side === 'left' && !same && text != null;
  const added = side === 'right' && !same && text != null;
  const gap = text == null; // el lado sin línea correspondiente (hueco de alineación)

  const bg = removed ? 'bg-error/10' : added ? 'bg-success/10' : gap ? 'bg-bg2/50' : '';
  const mark = removed
    ? 'border-l-2 border-error/50'
    : added
      ? 'border-l-2 border-success/50'
      : 'border-l-2 border-transparent';
  const divider = side === 'left' ? 'border-r border-hair2' : '';

  return (
    <div className={`grid grid-cols-[34px_1fr] ${divider} ${bg} ${mark}`}>
      <span className="select-none pr-2 text-right text-[10px] text-text3">{no ?? ''}</span>
      <span className="whitespace-pre pr-3">{text != null ? highlightAsmLine(text) : ''}</span>
    </div>
  );
}

function Swatch({ className }: { className: string }) {
  return <span className={`inline-block h-2 w-2 rounded-[3px] ${className}`} />;
}

function Counter({ label, v }: { label: string; v: number }) {
  return (
    <div className="flex items-center justify-between rounded-[7px] border border-hair2 bg-bg2 px-2.5 py-1.5 text-[11.5px]">
      <span className="text-text3">{label}</span>
      <span className={`font-mono ${v ? 'text-accent' : 'text-text3'}`}>{v}</span>
    </div>
  );
}

function Stat({ k, v, accent, title }: { k: string; v: string; accent?: boolean; title?: string }) {
  return (
    <div className="rounded-[10px] border border-hair bg-bg2 px-3 py-2" title={title}>
      <div className="text-[10px] uppercase tracking-[0.1em] text-text3">{k}</div>
      <div className={`mt-0.5 font-mono text-[15px] ${accent ? 'text-accent' : 'text-text'}`}>{v}</div>
    </div>
  );
}
