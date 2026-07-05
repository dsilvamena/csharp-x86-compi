// Declaraciones de tipos para simulator.js (que se mantiene en JavaScript puro).
// TypeScript toma este .d.ts hermano como los tipos del módulo, de modo que la
// app (TS) puede importar el simulador sin activar allowJs ni tipar la lógica.

export interface RegFile {
  rax: number; rbx: number; rcx: number; rdx: number;
  rsi: number; rdi: number; rbp: number; rsp: number;
  r8: number; r9: number; r10: number; r11: number;
  r12: number; r13: number; r14: number; r15: number;
  rip: number;
  [reg: string]: number;
}

export interface Flags {
  ZF: boolean; SF: boolean; OF: boolean; CF: boolean;
}

export interface MemCell { address: number; value: number; }
export interface DataEntry { label: string; address: number; value: string; }

/** Estado completo de la CPU virtual para la UI (devuelto por snapshot()). */
export interface Snapshot {
  regs: RegFile;
  flags: Flags;
  stack: MemCell[];
  heap: MemCell[];
  data: DataEntry[];
  output: string;
  currentLine: number | null;   // línea del .s de la próxima instrucción
  currentInstr: string | null;
  steps: number;
  halted: boolean;
  exitCode: number;
  error: string | null;         // p. ej. "límite de pasos alcanzado (...)"
}

export interface SimulatorOptions {
  maxSteps?: number;
}

export class X86AttSimulator {
  constructor(assemblyText: string, options?: SimulatorOptions);
  regs: RegFile;
  flags: Flags;
  output: string;
  steps: number;
  halted: boolean;
  exitCode: number;
  error: string | null;
  /** Ejecuta una instrucción (según %rip). Devuelve false al detenerse. */
  step(): boolean;
  /** Ejecuta hasta ret de main / halt. Devuelve el snapshot final. */
  run(): Snapshot;
  /** Reinicia la CPU sin re-parsear el ensamblador. */
  reset(): void;
  /** Estado completo actual para la UI. */
  snapshot(): Snapshot;
}

export default X86AttSimulator;
