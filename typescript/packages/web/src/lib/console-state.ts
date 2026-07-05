import type { Diagnostic } from './compiler-types';

export type ConsoleKind = 'initial' | 'success' | 'lexical' | 'syntax' | 'semantic';

export interface ConsoleState {
  kind: ConsoleKind;
  message: string;
}

// Console state strings use stable UI copy with the real line/column and
// diagnostic text filled in.
export function consoleState(source: string, result: { ok: boolean; diagnostics: Diagnostic[] }): ConsoleState {
  if (source.trim() === '') {
    return {
      kind: 'initial',
      message:
        'No compilation yet. Run the compiler to generate tokens, AST, symbols and x86 output.',
    };
  }
  if (result.ok) {
    return {
      kind: 'success',
      message: 'Compilation successful. Generated tokens, AST, symbol table and x86 assembly.',
    };
  }
  const err: Diagnostic | undefined =
    result.diagnostics.find((d) => d.severity === 'error') ?? result.diagnostics[0];
  if (!err) return { kind: 'initial', message: 'No compilation yet.' };

  if (err.phase === 'lexical')
    return { kind: 'lexical', message: `Lexical error. Line ${err.line}, column ${err.col}: ${err.message}` };
  if (err.phase === 'syntax')
    return { kind: 'syntax', message: `Syntax error. Line ${err.line}, column ${err.col}: ${err.message}` };
  return { kind: 'semantic', message: `Semantic error. Line ${err.line}: ${err.message}` };
}
