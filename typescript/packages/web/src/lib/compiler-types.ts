// Shapes of the JSON the C++ backend (`compiler --json`) returns. The compiler
// CLI is the single source of truth; this is the contract it
// emits — see c++/src/main.cpp (runJson) and backend/README.md.
//
// Intentionally absent (the CLI does not produce them): IR, optimizedIr,
// symbol table, and native stdout. The AST arrives as the textual tree from
// printAst (rendered as preformatted text), not a JSON tree.
//
// `asmSourceMap` IS emitted: one entry per line of `asm` (the --no-opt listing),
// holding the 1-based C# source line that produced it, or null. Powers the hover
// C#↔x86 highlight. See c++/src/main.cpp (stripLocMarkers / runJson).

export interface Loc {
  line: number;
  col: number;
}

/** Per-asm-line provenance: asmSourceMap[i] = C# source line of asm line i, or null. */
export type AsmSourceMap = (number | null)[];

// --- tokens --------------------------------------------------------------
// `type` is the CLI's Token::typeName string (e.g. "int", "ID", "==", "NUM").
export interface Token {
  type: string;
  lexeme: string;
  line: number;
  col: number;
  value?: number | string | boolean;
}

// --- diagnostics ---------------------------------------------------------
// Phases are mapped to English by the CLI (lexico→lexical, etc.).
export type Phase = 'lexical' | 'syntax' | 'semantic' | 'backend';
export type Severity = 'error' | 'warning';

export interface Diagnostic {
  phase: Phase;
  severity: Severity;
  message: string;
  line: number;
  col: number;
}

// --- optimization report -------------------------------------------------
// Real, measured data only: constant folding (ConstantVisitor) + peephole lines,
// plus the .s instruction count before (--no-opt) and after (--opt).
export interface OptReport {
  instructionsBefore: number;
  instructionsAfter: number;
  removed: number;
  reductionPercent: number;
  constantFolding: number;
  peepholeRemoved: number;
  summary: string;
}

/** Count real x86 instruction lines (excludes blanks, comments, directives, labels). */
export function asmInstructionCount(asm: string): number {
  return asm
    .split('\n')
    .map((l) => l.replace(/#.*/, '').trim())
    .filter((l) => l !== '' && l[0] !== '.' && !/:$/.test(l)).length;
}
