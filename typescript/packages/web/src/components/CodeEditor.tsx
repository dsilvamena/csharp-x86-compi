import { useEffect, useRef } from 'react';
import Editor, { type OnMount } from '@monaco-editor/react';
import type * as Monaco from 'monaco-editor';
import { LANG_ID, registerLimitedCSharp, THEME_ID } from '../editor/monaco-setup';

export function CodeEditor({
  value,
  onChange,
  hoverLine = null,
  onHoverLine = () => {},
  execLine = null,
}: {
  value: string;
  onChange: (v: string) => void;
  hoverLine?: number | null;
  onHoverLine?: (line: number | null) => void;
  /** Source line currently executing in the simulator (auto-followed). */
  execLine?: number | null;
}) {
  const editorRef = useRef<Monaco.editor.IStandaloneCodeEditor | null>(null);
  const monacoRef = useRef<typeof Monaco | null>(null);
  const decoRef = useRef<Monaco.editor.IEditorDecorationsCollection | null>(null);
  const execDecoRef = useRef<Monaco.editor.IEditorDecorationsCollection | null>(null);
  const lastLine = useRef<number | null>(null);
  // Keep the latest hover callback without re-registering Monaco listeners.
  const onHoverRef = useRef(onHoverLine);
  onHoverRef.current = onHoverLine;

  const onMount: OnMount = (editor, monaco) => {
    editorRef.current = editor;
    monacoRef.current = monaco;
    decoRef.current = editor.createDecorationsCollection();
    execDecoRef.current = editor.createDecorationsCollection();

    // Report the hovered source line (throttled to line changes) so the IR/x86
    // views can light up the instructions that line produced.
    editor.onMouseMove((e) => {
      const line = e.target.position?.lineNumber ?? null;
      if (line !== lastLine.current) {
        lastLine.current = line;
        onHoverRef.current(line);
      }
    });
    editor.onMouseLeave(() => {
      lastLine.current = null;
      onHoverRef.current(null);
    });
  };

  // When the hover originates elsewhere (IR/x86 view), decorate the matching source line.
  useEffect(() => {
    const coll = decoRef.current;
    const monaco = monacoRef.current;
    if (!coll || !monaco) return;
    if (hoverLine == null) {
      coll.clear();
      return;
    }
    coll.set([
      {
        range: new monaco.Range(hoverLine, 1, hoverLine, 1),
        options: { isWholeLine: true, className: 'prov-editor-line' },
      },
    ]);
  }, [hoverLine]);

  // Follow the simulator: highlight + reveal the line currently executing.
  useEffect(() => {
    const coll = execDecoRef.current;
    const monaco = monacoRef.current;
    if (!coll || !monaco) return;
    if (execLine == null) {
      coll.clear();
      return;
    }
    coll.set([
      {
        range: new monaco.Range(execLine, 1, execLine, 1),
        options: { isWholeLine: true, className: 'exec-editor-line', glyphMarginClassName: 'exec-editor-glyph' },
      },
    ]);
    editorRef.current?.revealLineInCenterIfOutsideViewport(execLine);
  }, [execLine]);

  return (
    <Editor
      language={LANG_ID}
      theme={THEME_ID}
      value={value}
      beforeMount={(m) => registerLimitedCSharp(m)}
      onMount={onMount}
      onChange={(v) => onChange(v ?? '')}
      loading={<div className="p-4 text-[13px] text-text3">Loading editor…</div>}
      options={{
        fontFamily: 'Geist Mono, ui-monospace, monospace',
        fontSize: 12.5,
        lineHeight: 22,
        minimap: { enabled: false },
        glyphMargin: true, // gutter marker for the executing line
        automaticLayout: true,
        scrollBeyondLastLine: false,
        padding: { top: 12, bottom: 12 },
        renderLineHighlight: 'line',
        smoothScrolling: true,
        cursorSmoothCaretAnimation: 'on',
        scrollbar: { verticalScrollbarSize: 9, horizontalScrollbarSize: 9 },
        overviewRulerLanes: 0,
        guides: { indentation: false },
        tabSize: 2,
        fontLigatures: false,
        contextmenu: false,
      }}
    />
  );
}
