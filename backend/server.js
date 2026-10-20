// Local development bridge. Production uses api/compile.js on Vercel.
import express from 'express';
import { compileHandler } from './compiler.js';

const app = express();
const port = Number(process.env.PORT || 8787);
app.use(express.json({ limit: '1mb' }));
app.use(express.text({ type: ['text/plain', 'application/x-csharp'], limit: '1mb' }));
app.all('/api/compile', compileHandler);
app.get('/api/health', (_req, res) => res.json({ ok: true }));
app.listen(port, () => console.log(`[backend] escuchando en http://localhost:${port}`));
