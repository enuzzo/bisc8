#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";

const defaults = {
  model: "gpt-4o-mini-tts",
  voice: "ash",
  input: "La visione si apre. Una sola frase, breve e misteriosa.",
  instructions: "Parla come fossi un mago che recita una profezia misteriosa.",
  format: "pcm",
  out: "/tmp/bisc8-speech-answer.pcm",
};

function argValue(name) {
  const prefix = `--${name}=`;
  const exact = `--${name}`;
  for (let i = 2; i < process.argv.length; ++i) {
    if (process.argv[i].startsWith(prefix)) {
      return process.argv[i].slice(prefix.length);
    }
    if (process.argv[i] === exact && i + 1 < process.argv.length) {
      return process.argv[i + 1];
    }
  }
  return defaults[name];
}

function loadApiKey() {
  if (process.env.OPENAI_API_KEY) {
    return process.env.OPENAI_API_KEY;
  }
  const envPath = path.resolve(".env.local");
  const text = fs.readFileSync(envPath, "utf8");
  for (const line of text.split(/\r?\n/)) {
    const match = line.match(/^\s*OPENAI_API_KEY\s*=\s*(.+?)\s*$/);
    if (match) {
      return match[1].replace(/^['"]|['"]$/g, "");
    }
  }
  throw new Error("OPENAI_API_KEY not found in environment or .env.local");
}

function readWavInfo(buf) {
  const info = {
    riff: buf.toString("ascii", 0, 4),
    wave: buf.toString("ascii", 8, 12),
    chunks: [],
    fmt: null,
    data: null,
  };
  if (buf.length < 12) {
    return info;
  }
  let p = 12;
  while (p + 8 <= buf.length && info.chunks.length < 32) {
    const id = buf.toString("ascii", p, p + 4);
    const size = buf.readUInt32LE(p + 4);
    info.chunks.push({ id, off: p, size });
    if (id === "fmt " && p + 24 <= buf.length) {
      info.fmt = {
        audioFormat: buf.readUInt16LE(p + 8),
        channels: buf.readUInt16LE(p + 10),
        rate: buf.readUInt32LE(p + 12),
        byteRate: buf.readUInt32LE(p + 16),
        blockAlign: buf.readUInt16LE(p + 20),
        bits: buf.readUInt16LE(p + 22),
        fmtSize: size,
      };
    }
    if (id === "data") {
      info.data = { off: p + 8, size };
      break;
    }
    p += 8 + size + (size & 1);
  }
  return info;
}

const body = {
  model: argValue("model"),
  voice: argValue("voice"),
  input: argValue("input"),
  instructions: argValue("instructions"),
  response_format: argValue("format"),
};

const out = argValue("out");
const apiKey = loadApiKey();
const t0 = Date.now();
const res = await fetch("https://api.openai.com/v1/audio/speech", {
  method: "POST",
  headers: {
    Authorization: `Bearer ${apiKey}`,
    "Content-Type": "application/json",
  },
  body: JSON.stringify(body),
});
const buf = Buffer.from(await res.arrayBuffer());
console.log(JSON.stringify({
  status: res.status,
  contentType: res.headers.get("content-type"),
  bytes: buf.length,
  ms: Date.now() - t0,
}));
if (!res.ok) {
  console.error(buf.toString("utf8", 0, Math.min(buf.length, 500)));
  process.exit(2);
}
fs.writeFileSync(out, buf);
if (body.response_format === "wav") {
  console.log(JSON.stringify({ file: out, wav: readWavInfo(buf) }, null, 2));
} else {
  console.log(JSON.stringify({
    file: out,
    audio: {
      format: body.response_format,
      rate: 24000,
      channels: 1,
      bits: 16,
      secondsApprox: Math.round((buf.length / 48000) * 10) / 10,
    },
  }, null, 2));
}
