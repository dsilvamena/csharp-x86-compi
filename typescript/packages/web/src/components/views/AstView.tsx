// Vista AST. El CLI expone el AST como el árbol textual de printAst (una cadena
// con 2 espacios de sangría por nivel: «<Tipo> <detalle>»). Aquí lo parseamos de
// vuelta a un árbol y lo mostramos como un árbol interactivo (plegable, con
// guías, tipos coloreados por categoría) en vez de texto plano. Se conserva el
// modo «Texto» por si se quiere el volcado crudo.
import { useMemo, useState } from 'react';
import { ChevronRight, ListTree } from 'lucide-react';
import { Empty, PanelLabel } from '../primitives';

interface AstNode {
  kind: string;
  detail: string;
  children: AstNode[];
  path: string;
}

type Cat = 'decl' | 'stmt' | 'expr' | 'lit' | 'ref';

// Categoría de cada tipo de nodo (para el color). Los nombres coinciden con los
// que emite printAst (c++/src/ast.cpp).
const CAT: Record<string, Cat> = {
  Program: 'decl', Class: 'decl', Struct: 'decl', Method: 'decl',
  Block: 'stmt', VarDec: 'stmt', Assign: 'stmt', ExprStm: 'stmt', Print: 'stmt',
  If: 'stmt', While: 'stmt', DoWhile: 'stmt', For: 'stmt', Return: 'stmt',
  Break: 'stmt', Unsafe: 'stmt',
  Binary: 'expr', Unary: 'expr', Call: 'expr', Cast: 'expr', AddressOf: 'expr',
  Deref: 'expr', NewArray: 'expr', Index: 'expr', NewMatrix: 'expr',
  MatrixIndex: 'expr', Field: 'expr',
  Number: 'lit', Bool: 'lit', Char: 'lit', String: 'lit',
  Id: 'ref',
};

const CAT_COLOR: Record<Cat, string> = {
  decl: '#8b7cf6', // morado (declaraciones / estructura)
  stmt: '#5aa9e6', // azul (sentencias)
  expr: '#4fb872', // verde (expresiones)
  lit: '#e0a458',  // ámbar (literales)
  ref: '#e0709a',  // rosa (identificadores)
};

const CAT_LABEL: Record<Cat, string> = {
  decl: 'Declaración', stmt: 'Sentencia', expr: 'Expresión', lit: 'Literal', ref: 'Identificador',
};

function catOf(kind: string): Cat {
  return CAT[kind] ?? 'expr';
}

// Parsea el árbol textual (sangría de 2 espacios) en nodos anidados.
function parseAst(text: string): AstNode[] {
  const roots: AstNode[] = [];
  const stack: { node: AstNode; depth: number }[] = [];
  let counter = 0;
  for (const raw of text.split('\n')) {
    if (!raw.trim()) continue;
    const m = raw.match(/^( *)(\S.*)$/);
    if (!m) continue;
    const depth = Math.floor(m[1].length / 2);
    const content = m[2];
    const sp = content.indexOf(' ');
    const kind = sp === -1 ? content : content.slice(0, sp);
    const detail = sp === -1 ? '' : content.slice(sp + 1).trim();
    const node: AstNode = { kind, detail, children: [], path: String(counter++) };
    while (stack.length && stack[stack.length - 1].depth >= depth) stack.pop();
    if (stack.length === 0) roots.push(node);
    else stack[stack.length - 1].node.children.push(node);
    stack.push({ node, depth });
  }
  return roots;
}

function countNodes(nodes: AstNode[]): number {
  return nodes.reduce((n, x) => n + 1 + countNodes(x.children), 0);
}

function internalPaths(nodes: AstNode[], acc: string[] = []): string[] {
  for (const n of nodes) {
    if (n.children.length) { acc.push(n.path); internalPaths(n.children, acc); }
  }
  return acc;
}

export function AstView({ ast }: { ast: string }) {
  const roots = useMemo(() => parseAst(ast), [ast]);
  const total = useMemo(() => countNodes(roots), [roots]);
  const [mode, setMode] = useState<'tree' | 'text'>('tree');
  // Nodos plegados (por path). Por defecto todo expandido.
  const [collapsed, setCollapsed] = useState<Set<string>>(() => new Set());

  if (!ast.trim()) return <Empty>Sin AST — corrige los errores para parsear el programa.</Empty>;

  const toggle = (path: string) =>
    setCollapsed((prev) => {
      const next = new Set(prev);
      if (next.has(path)) next.delete(path); else next.add(path);
      return next;
    });
  const collapseAll = () => setCollapsed(new Set(internalPaths(roots)));
  const expandAll = () => setCollapsed(new Set());

  return (
    <div className="flex h-full min-h-0 flex-col">
      {/* Barra de control */}
      <div className="flex items-center justify-between gap-2 border-b border-hair2 bg-bg2 px-3.5 py-2">
        <div className="flex items-center gap-2">
          <PanelLabel>Árbol de sintaxis abstracta</PanelLabel>
          <span className="font-mono text-[10.5px] text-text3">{total} nodos</span>
        </div>
        <div className="flex items-center gap-2">
          {mode === 'tree' && (
            <>
              <MiniBtn onClick={expandAll}>Expandir</MiniBtn>
              <MiniBtn onClick={collapseAll}>Colapsar</MiniBtn>
              <span className="h-4 w-px bg-hair" />
            </>
          )}
          <div className="flex rounded-[6px] border border-hair p-0.5 text-[11px]">
            {(['tree', 'text'] as const).map((m) => (
              <button
                key={m}
                onClick={() => setMode(m)}
                className={`rounded-[4px] px-2 py-0.5 transition-colors ${
                  mode === m ? 'bg-accent/15 text-accent' : 'text-text3 hover:text-text2'
                }`}
              >
                {m === 'tree' ? 'Árbol' : 'Texto'}
              </button>
            ))}
          </div>
        </div>
      </div>

      {mode === 'text' ? (
        <div className="scroll-thin min-h-0 flex-1 overflow-auto p-3.5 font-mono text-[12px] leading-[1.7]">
          <pre className="whitespace-pre text-text2">{ast}</pre>
        </div>
      ) : (
        <div className="scroll-thin min-h-0 flex-1 overflow-auto p-3 font-mono text-[12.5px] leading-[1.5]">
          {roots.map((n) => (
            <TreeNode key={n.path} node={n} collapsed={collapsed} onToggle={toggle} />
          ))}
          <Legend />
        </div>
      )}
    </div>
  );
}

function TreeNode({
  node, collapsed, onToggle,
}: {
  node: AstNode; collapsed: Set<string>; onToggle: (p: string) => void;
}) {
  const hasChildren = node.children.length > 0;
  const open = !collapsed.has(node.path);
  const cat = catOf(node.kind);
  const color = CAT_COLOR[cat];

  return (
    <div>
      <div
        className={`group flex items-center gap-1.5 rounded-[5px] py-[2px] pr-2 ${
          hasChildren ? 'cursor-pointer hover:bg-elev/60' : ''
        }`}
        onClick={hasChildren ? () => onToggle(node.path) : undefined}
      >
        <span className="flex h-4 w-4 shrink-0 items-center justify-center text-text3">
          {hasChildren ? (
            <ChevronRight
              size={13}
              className="transition-transform duration-150"
              style={{ transform: open ? 'rotate(90deg)' : 'none' }}
            />
          ) : (
            <span className="h-1 w-1 rounded-full bg-hair" />
          )}
        </span>
        <span
          className="shrink-0 rounded-[5px] px-1.5 py-px text-[11px] font-medium"
          style={{ color, background: `${color}1f`, border: `1px solid ${color}3a` }}
          title={CAT_LABEL[cat]}
        >
          {node.kind}
        </span>
        {node.detail && <span className="truncate text-text2">{node.detail}</span>}
        {hasChildren && !open && (
          <span className="font-mono text-[10.5px] text-text3">· {node.children.length}</span>
        )}
      </div>

      {hasChildren && open && (
        <div className="ml-[7px] border-l border-hair2 pl-[11px]">
          {node.children.map((c) => (
            <TreeNode key={c.path} node={c} collapsed={collapsed} onToggle={onToggle} />
          ))}
        </div>
      )}
    </div>
  );
}

function Legend() {
  return (
    <div className="mt-4 flex flex-wrap items-center gap-x-3 gap-y-1 border-t border-hair2 pt-3 text-[10.5px] text-text3">
      <ListTree size={12} />
      {(Object.keys(CAT_LABEL) as Cat[]).map((c) => (
        <span key={c} className="flex items-center gap-1">
          <span className="inline-block h-2 w-2 rounded-[3px]" style={{ background: CAT_COLOR[c] }} />
          {CAT_LABEL[c]}
        </span>
      ))}
    </div>
  );
}

function MiniBtn({ onClick, children }: { onClick: () => void; children: React.ReactNode }) {
  return (
    <button
      onClick={onClick}
      className="rounded-[5px] border border-hair px-2 py-0.5 text-[11px] text-text2 transition-colors hover:text-text"
    >
      {children}
    </button>
  );
}
