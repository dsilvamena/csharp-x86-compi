import type { ReactNode } from 'react';

const INSTRS = new Set([
  'mov', 'movsd', 'lea', 'push', 'pop', 'add', 'sub', 'imul', 'idiv', 'mul', 'div',
  'addsd', 'subsd', 'mulsd', 'divsd', 'cvtsi2sd', 'cvttsd2si', 'comisd', 'and', 'or',
  'xor', 'neg', 'cmp', 'test', 'jmp', 'je', 'jne', 'jl', 'jg', 'jle', 'jge', 'jz', 'jnz',
  'call', 'ret', 'leave', 'cqo', 'cdq',
]);

const REGS =
  /^(rax|rbx|rcx|rdx|rsi|rdi|rbp|rsp|r8|r9|r10|r11|r12|r13|r14|r15|eax|ebx|ecx|edx|al|bl|cl|dl|xmm\d+)$/;

/** Render one assembly line with mockup-matched token colors. */
export function highlightAsmLine(line: string): ReactNode {
  const trimmed = line.trimStart();

  // Section directives / comments.
  if (trimmed.startsWith(';')) return <span className="text-com italic">{line}</span>;
  if (trimmed.startsWith('section') || trimmed.startsWith('global') || trimmed.startsWith('default') ||
      trimmed.startsWith('extern') || trimmed.startsWith('bits')) {
    return <span className="text-text3">{line}</span>;
  }
  // Label definition (e.g. "fib:", ".main_L0:", "msg db ...").
  if (/^[.\w]+:/.test(trimmed)) {
    const idx = line.indexOf(':');
    return (
      <>
        <span className="text-type">{line.slice(0, idx + 1)}</span>
        <span className="text-text2">{line.slice(idx + 1)}</span>
      </>
    );
  }

  const indent = line.slice(0, line.length - trimmed.length);
  const spaceIdx = trimmed.indexOf(' ');
  const mnemonic = spaceIdx === -1 ? trimmed : trimmed.slice(0, spaceIdx);
  const rest = spaceIdx === -1 ? '' : trimmed.slice(spaceIdx);

  const isInstr = INSTRS.has(mnemonic);
  // Data directives (db/dq/dd/resq…) — show like declarations.
  const isData = /^(db|dq|dd|dw|resq|resd|resb|equ|times)$/.test(mnemonic);

  return (
    <>
      <span>{indent}</span>
      <span className={isInstr ? 'text-text' : isData ? 'text-text3' : 'text-text2'}>{mnemonic}</span>
      {highlightOperands(rest)}
    </>
  );
}

function highlightOperands(rest: string): ReactNode {
  if (!rest) return null;
  // Split on commas keeping the separators, then tokenize each operand piece.
  const parts = rest.split(/([,\s])/);
  return parts.map((p, i) => {
    if (p === '' || /^[,\s]$/.test(p)) return <span key={i} className="text-text2">{p}</span>;
    // Memory operand [rbp-8]
    if (p.startsWith('[') || p.endsWith(']')) return <span key={i} className="text-str">{p}</span>;
    // Immediate number
    if (/^-?\d/.test(p)) return <span key={i} className="text-num">{p}</span>;
    // Register
    if (REGS.test(p)) return <span key={i} className="text-text2">{p}</span>;
    // Label / symbol target
    return <span key={i} className="text-type">{p}</span>;
  });
}
