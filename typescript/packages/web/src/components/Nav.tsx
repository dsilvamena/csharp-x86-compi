import { Play, Sparkles } from 'lucide-react';

export function Nav({ onRun, onOptimize }: { onRun: () => void; onOptimize: () => void }) {
  return (
    <div className="sticky top-0 z-30 border-b border-hair backdrop-blur-md" style={{ background: 'color-mix(in srgb, var(--color-bg) 86%, transparent)' }}>
      <div className="mx-auto flex h-14 max-w-[1320px] items-center justify-between px-7">
        <div className="flex items-center gap-2.5">
          <div
            className="grid h-[26px] w-[26px] place-items-center rounded-[7px] font-mono text-[13px] font-semibold text-white"
            style={{
              background: 'linear-gradient(140deg, var(--color-accent), color-mix(in srgb, var(--color-accent) 40%, #000))',
              boxShadow: '0 0 0 1px var(--border), 0 6px 16px -8px var(--accent-glow)',
            }}
          >
            {'>_'}
          </div>
          <b className="text-[14px] font-semibold tracking-[-0.01em]">C# x86 Compiler Studio</b>
          <span className="rounded-full border border-hair px-2.5 py-0.5 text-[11px] tracking-[0.02em] text-text2">
            limited C#
          </span>
        </div>
        <div className="flex items-center gap-2.5">
          <button
            onClick={onOptimize}
            className="flex items-center gap-1.5 rounded-[8px] border border-hair bg-panel2 px-3 py-[7px] text-[13px] font-medium text-text transition-colors duration-150 hover:bg-elev"
          >
            <Sparkles size={14} /> Optimize
          </button>
          <button
            onClick={onRun}
            className="flex items-center gap-1.5 rounded-[8px] bg-accent px-3.5 py-[7px] text-[13px] font-semibold text-[#0A0B0D] transition-all duration-150 hover:bg-accent-hover active:scale-[0.98]"
            style={{ boxShadow: '0 6px 18px -8px var(--accent-glow)' }}
          >
            <Play size={13} /> Run
          </button>
        </div>
      </div>
    </div>
  );
}
