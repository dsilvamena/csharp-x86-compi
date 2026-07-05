import { loader } from '@monaco-editor/react';
import type * as Monaco from 'monaco-editor';
// Import only the editor API (not the ~90 bundled languages) — we register our own
// limited-C# language, so this keeps the offline bundle small. Types come from the
// package root, the runtime value from the slim ESM entry.
// @ts-expect-error — no type declarations ship for this deep ESM path.
import * as monacoApi from 'monaco-editor/esm/vs/editor/editor.api';
import editorWorker from 'monaco-editor/esm/vs/editor/editor.worker?worker';

const monaco = monacoApi as typeof Monaco;

// Bundle Monaco from npm (no CDN) so the app works offline / on a static host.
// The limited C# has no JSON/CSS/TS features, so one editor worker is enough.
self.MonacoEnvironment = {
  getWorker() {
    return new editorWorker();
  },
};
loader.config({ monaco });

export const LANG_ID = 'limited-csharp';
export const THEME_ID = 'studio-violet';

let registered = false;

/** Register the limited-C# language + the violet editor theme (idempotent). */
export function registerLimitedCSharp(m: typeof monaco) {
  if (registered) return;
  registered = true;

  m.languages.register({ id: LANG_ID });

  m.languages.setMonarchTokensProvider(LANG_ID, {
    keywords: ['if', 'else', 'while', 'for', 'return', 'struct', 'new', 'print', 'true', 'false'],
    typeKeywords: ['int', 'float', 'bool', 'string', 'void'],
    operators: [
      '=', '==', '!=', '<', '<=', '>', '>=', '+', '-', '*', '/', '%', '&&', '||', '!', '&',
    ],
    symbols: /[=><!~?:&|+\-*/^%]+/,
    tokenizer: {
      root: [
        [/\/\/.*$/, 'comment'],
        [/\/\*/, 'comment', '@comment'],
        [/"([^"\\]|\\.)*"/, 'string'],
        [/\b\d+\.\d+\b/, 'number.float'],
        [/\b\d+\b/, 'number'],
        [
          /[a-zA-Z_]\w*/,
          {
            cases: {
              '@typeKeywords': 'type',
              '@keywords': 'keyword',
              '@default': 'identifier',
            },
          },
        ],
        [/[a-zA-Z_]\w*(?=\s*\()/, 'function'],
        [/[{}()[\]]/, 'delimiter'],
        [/[;,.]/, 'delimiter'],
        [
          /@symbols/,
          { cases: { '@operators': 'operator', '@default': 'delimiter' } },
        ],
      ],
      comment: [
        [/[^/*]+/, 'comment'],
        [/\*\//, 'comment', '@pop'],
        [/[/*]/, 'comment'],
      ],
    },
  });

  m.editor.defineTheme(THEME_ID, {
    base: 'vs-dark',
    inherit: true,
    rules: [
      { token: 'keyword', foreground: '8B7CF6' },
      { token: 'type', foreground: '5CC9C0' },
      { token: 'string', foreground: '9DCE6A' },
      { token: 'number', foreground: 'E0A458' },
      { token: 'number.float', foreground: 'E0A458' },
      { token: 'comment', foreground: '5E626B', fontStyle: 'italic' },
      { token: 'function', foreground: '6FA8FF' },
      { token: 'identifier', foreground: 'ECEDEF' },
      { token: 'operator', foreground: '9A9DA5' },
      { token: 'delimiter', foreground: '9A9DA5' },
    ],
    colors: {
      'editor.background': '#111316',
      'editor.foreground': '#ECEDEF',
      'editorLineNumber.foreground': '#686B73',
      'editorLineNumber.activeForeground': '#9A9DA5',
      'editor.lineHighlightBackground': '#16181C',
      'editor.selectionBackground': '#8B7CF633',
      'editorCursor.foreground': '#8B7CF6',
      'editorIndentGuide.background1': '#ffffff10',
      'editorIndentGuide.activeBackground1': '#ffffff20',
      'editorWidget.background': '#16181C',
      'editorWidget.border': '#ffffff12',
      'editorGutter.background': '#111316',
      'scrollbarSlider.background': '#ffffff14',
      'scrollbarSlider.hoverBackground': '#ffffff20',
    },
  });
}
