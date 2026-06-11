#!/usr/bin/env node

import { writeFileSync } from "node:fs";
import { Buffer } from "node:buffer";

const defaults = {
  model: "gpt-realtime-2",
  voice: "cedar",
  out: "/tmp/bisc8-realtime-answer.wav",
  text: "Speak one short mystical sentence for a pocket oracle.",
};

function usage() {
  console.log(`Usage:
  OPENAI_API_KEY=sk-... node tools/realtime_tts_smoke.mjs [options]

Options:
  --model <id>   Realtime model (default: ${defaults.model})
  --voice <id>   Realtime voice (default: ${defaults.voice})
  --text <text>  Text to speak
  --out <path>   WAV output path (default: ${defaults.out})
  --help         Show this help
`);
}

function parseArgs(argv) {
  const opts = { ...defaults };
  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    if (arg === "--help" || arg === "-h") {
      opts.help = true;
    } else if (arg === "--model") {
      opts.model = argv[++i];
    } else if (arg === "--voice") {
      opts.voice = argv[++i];
    } else if (arg === "--text") {
      opts.text = argv[++i];
    } else if (arg === "--out") {
      opts.out = argv[++i];
    } else {
      throw new Error(`Unknown argument: ${arg}`);
    }
  }
  return opts;
}

function wavHeader(pcmBytes, sampleRate = 24000) {
  const header = Buffer.alloc(44);
  header.write("RIFF", 0);
  header.writeUInt32LE(36 + pcmBytes, 4);
  header.write("WAVEfmt ", 8);
  header.writeUInt32LE(16, 16);
  header.writeUInt16LE(1, 20);
  header.writeUInt16LE(1, 22);
  header.writeUInt32LE(sampleRate, 24);
  header.writeUInt32LE(sampleRate * 2, 28);
  header.writeUInt16LE(2, 32);
  header.writeUInt16LE(16, 34);
  header.write("data", 36);
  header.writeUInt32LE(pcmBytes, 40);
  return header;
}

async function messageText(data) {
  if (typeof data === "string") {
    return data;
  }
  if (data instanceof Buffer) {
    return data.toString("utf8");
  }
  if (data instanceof ArrayBuffer) {
    return Buffer.from(data).toString("utf8");
  }
  if (typeof data?.text === "function") {
    return data.text();
  }
  return String(data);
}

async function main() {
  const opts = parseArgs(process.argv.slice(2));
  if (opts.help) {
    usage();
    return;
  }
  const apiKey = process.env.OPENAI_API_KEY;
  if (!apiKey) {
    throw new Error("OPENAI_API_KEY is not set");
  }

  const url = `wss://api.openai.com/v1/realtime?model=${encodeURIComponent(opts.model)}`;
  const ws = new WebSocket(url, {
    headers: {
      Authorization: `Bearer ${apiKey}`,
      "User-Agent": "bisc8-realtime-smoke/1.0",
    },
  });

  const chunks = [];
  let audioBytes = 0;
  let done = false;
  let firstAudio = false;

  await new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      ws.close();
      reject(new Error("Timed out waiting for Realtime response"));
    }, 45000);
    const fail = (error) => {
      clearTimeout(timeout);
      reject(error);
    };

    ws.addEventListener("open", () => {
      console.log(`[smoke] connected model=${opts.model} voice=${opts.voice}`);
    });

    ws.addEventListener("error", (event) => {
      fail(new Error(`WebSocket error: ${event.message || "unknown"}`));
    });

    ws.addEventListener("close", () => {
      if (!done) {
        fail(new Error("WebSocket closed before response.done"));
      }
    });

    ws.addEventListener("message", async (event) => {
      try {
        const data = JSON.parse(await messageText(event.data));
        if (data.type === "session.created") {
          console.log("[smoke] session.created");
          ws.send(JSON.stringify({
            type: "conversation.item.create",
            item: {
              type: "message",
              role: "user",
              content: [{ type: "input_text", text: opts.text }],
            },
          }));
          ws.send(JSON.stringify({
            type: "response.create",
            response: {
              output_modalities: ["audio"],
              audio: {
                output: {
                  format: { type: "audio/pcm", rate: 24000 },
                  voice: opts.voice,
                },
              },
              instructions: "Speak exactly the provided text in a calm, theatrical oracle voice.",
            },
          }));
          return;
        }

        if (data.type === "response.audio.delta" || data.type === "response.output_audio.delta") {
          const pcm = Buffer.from(data.delta || "", "base64");
          chunks.push(pcm);
          audioBytes += pcm.length;
          if (!firstAudio) {
            firstAudio = true;
            console.log(`[smoke] first audio chunk=${pcm.length}B`);
          }
          return;
        }

        if (data.type === "response.audio.done" || data.type === "response.output_audio.done") {
          console.log(`[smoke] audio.done bytes=${audioBytes}`);
          return;
        }

        if (data.type === "response.done") {
          const status = data.response?.status || "unknown";
          console.log(`[smoke] response.done status=${status} bytes=${audioBytes}`);
          if (status !== "completed" || audioBytes === 0) {
            fail(new Error(`Realtime response failed status=${status} bytes=${audioBytes}`));
            return;
          }
          const pcm = Buffer.concat(chunks);
          writeFileSync(opts.out, Buffer.concat([wavHeader(pcm.length), pcm]));
          console.log(`[smoke] wrote ${opts.out}`);
          done = true;
          clearTimeout(timeout);
          ws.close();
          resolve();
          return;
        }

        if (data.type === "error") {
          fail(new Error(`Realtime error: ${data.error?.message || JSON.stringify(data.error)}`));
        }
      } catch (error) {
        fail(error);
      }
    });
  });
}

main().catch((error) => {
  console.error(`[smoke] ${error.message}`);
  process.exitCode = 1;
});
