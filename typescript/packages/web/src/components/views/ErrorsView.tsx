import { CheckCircle2, XCircle, AlertTriangle } from 'lucide-react';
import type { Diagnostic } from '../../lib/compiler-types';

const PHASE_LABEL: Record<string, string> = {
  lexical: 'Lexical',
  syntax: 'Syntax',
  semantic: 'Semantic',
};

export function ErrorsView({ diagnostics }: { diagnostics: Diagnostic[] }) {
  if (diagnostics.length === 0) {
    return (
      <div className="flex h-full min-h-[120px] flex-col items-center justify-center gap-2 text-center">
        <CheckCircle2 size={22} className="text-success" />
        <div className="text-[13px] text-text2">
          No errors. Lexical, syntactic and semantic analysis succeeded.
        </div>
      </div>
    );
  }
  return (
    <div className="scroll-thin h-full overflow-auto p-3 ">
      <div className="flex flex-col gap-2">
        {diagnostics.map((d, i) => {
          const isError = d.severity === 'error';
          return (
            <div
              key={i}
              className={`flex gap-2.5 rounded-[9px] border bg-bg2 px-3 py-2.5 ${
                isError ? 'border-error/30' : 'border-warning/30'
              }`}
            >
              {isError ? (
                <XCircle size={15} className="mt-0.5 shrink-0 text-error" />
              ) : (
                <AlertTriangle size={15} className="mt-0.5 shrink-0 text-warning" />
              )}
              <div className="text-[12.5px]">
                <div className="mb-0.5 flex items-center gap-2">
                  <span
                    className={`rounded-[4px] border px-1.5 text-[10px] uppercase tracking-wide ${
                      isError ? 'border-error/40 text-error' : 'border-warning/40 text-warning'
                    }`}
                  >
                    {PHASE_LABEL[d.phase] ?? d.phase}
                  </span>
                  <span className="font-mono text-text3">
                    line {d.line}, column {d.col}
                  </span>
                </div>
                <div className="text-text">{d.message}</div>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}
