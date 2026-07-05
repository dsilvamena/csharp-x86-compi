import { asmInstructionCount, type OptReport } from '../lib/compiler-types';

export function OptStrip({
  report,
  asm,
  asmOpt,
}: {
  report: OptReport;
  asm: string | null;
  asmOpt: string | null;
}) {
  const x86Before = asm ? asmInstructionCount(asm) : report.instructionsBefore;
  const x86After = asmOpt ? asmInstructionCount(asmOpt) : report.instructionsAfter;
  return (
    <div
      className="mb-10 flex flex-wrap items-center justify-between gap-4 rounded-[13px] border border-hair px-[18px] py-4"
      style={{ background: 'linear-gradient(180deg, var(--color-panel), var(--color-panel2))' }}
    >
      <div className="text-[13px] text-text2">
        <b className="font-semibold text-text">Reporte de optimización.</b> &nbsp;Constantes
        plegadas: {report.constantFolding} &nbsp;|&nbsp; Líneas peephole: {report.peepholeRemoved}
      </div>
      <div className="flex gap-2.5">
        <Stat
          k="Instr. .s"
          v={`${x86Before} → ${x86After}`}
          title="Instrucciones x86 reales antes → después (excluye blancos, comentarios, directivas y etiquetas)."
        />
        <Stat
          k="Mejora"
          v={`${report.reductionPercent}%`}
          accent
          title="Reducción de instrucciones x86: eliminadas / antes × 100."
        />
      </div>
    </div>
  );
}

function Stat({ k, v, accent, title }: { k: string; v: string; accent?: boolean; title?: string }) {
  return (
    <div className="min-w-[96px] rounded-[10px] border border-hair bg-bg2 px-3.5 py-2" title={title}>
      <div className="text-[10px] uppercase tracking-[0.1em] text-text3">{k}</div>
      <div className={`mt-[3px] font-mono text-[16px] ${accent ? 'text-accent' : 'text-text'}`}>{v}</div>
    </div>
  );
}
