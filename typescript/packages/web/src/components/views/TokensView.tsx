import type { Token } from '../../lib/compiler-types';
import { Empty } from '../primitives';

// Categorize a token type for color, mirroring the editor's syntax palette.
function tokenClass(t: Token): string {
  const ty = t.type;
  if (['int', 'float', 'bool', 'string', 'void'].includes(ty)) return 'text-type';
  if (['if', 'else', 'while', 'for', 'return', 'struct', 'new', 'print', 'true', 'false'].includes(ty))
    return 'text-kw';
  if (ty === 'int_literal' || ty === 'float_literal') return 'text-num';
  if (ty === 'string_literal') return 'text-str';
  if (ty === 'identifier') return 'text-text';
  if (ty === 'eof') return 'text-text3';
  return 'text-punct';
}

export function TokensView({ tokens }: { tokens: Token[] }) {
  if (!tokens.length) return <Empty>No tokens.</Empty>;
  return (
    <div className="scroll-thin h-full overflow-auto">
      <table className="w-full border-collapse font-mono text-[12px]">
        <thead className="sticky top-0 z-10 bg-panel2 text-[10.5px] uppercase tracking-[0.12em] text-text3">
          <tr>
            <th className="px-3.5 py-2 text-left font-medium">#</th>
            <th className="px-3.5 py-2 text-left font-medium">Type</th>
            <th className="px-3.5 py-2 text-left font-medium">Lexeme</th>
            <th className="px-3.5 py-2 text-right font-medium">Line:Col</th>
          </tr>
        </thead>
        <tbody>
          {tokens.map((t, i) => (
            <tr key={i} className="border-b border-hair2 transition-colors hover:bg-panel2">
              <td className="px-3.5 py-1.5 text-text3">{i}</td>
              <td className={`px-3.5 py-1.5 ${tokenClass(t)}`}>{t.type}</td>
              <td className="px-3.5 py-1.5 text-text">{t.type === 'eof' ? '—' : t.lexeme}</td>
              <td className="px-3.5 py-1.5 text-right text-text3">
                {t.line}:{t.col}
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
