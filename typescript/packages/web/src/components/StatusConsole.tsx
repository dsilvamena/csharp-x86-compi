import { AlertCircle, CheckCircle2, Terminal } from 'lucide-react';
import type { ConsoleState } from '../lib/console-state';

export function StatusConsole({ state, busy = false }: { state: ConsoleState; busy?: boolean }) {
  const ok = state.kind === 'success';
  const err = state.kind === 'lexical' || state.kind === 'syntax' || state.kind === 'semantic';
  const Icon = ok ? CheckCircle2 : err ? AlertCircle : Terminal;
  const color = ok ? 'text-success' : err ? 'text-error' : 'text-text3';
  return (
    <div className="flex items-center gap-2.5 rounded-[10px] border border-hair bg-panel px-3.5 py-2.5 font-mono text-[12.5px]">
      <Icon size={15} className={`shrink-0 ${color}`} />
      <span className={ok ? 'text-text2' : err ? 'text-text' : 'text-text3'}>{state.message}</span>
      {busy && <span className="ml-auto text-[11px] text-text3">compilando…</span>}
    </div>
  );
}
