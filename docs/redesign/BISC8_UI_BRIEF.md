# Bisc8 UI Brief (for Claude Code)

You are entering an EXISTING repository. Your job is a UI takeover, not a rewrite.
Read this whole file before touching code, then follow the validation loop at the bottom.

## 0. Mission and scope

Take control of the user interface of two surfaces:

1. The device screen (Waveshare ESP32-C6-ePaper-1.54, e-paper, 200x200, 1-bit).
2. The web configuration interface (mobile-first, viewed on an iPhone).

Hard rules:

- Do NOT revolutionize the existing logic. Preserve the firmware behavior, the data model, the responsi storage format, the networking, and the OTA mechanism.
- You are only taking control of how it looks and reads. Touch UI code and UI-adjacent code only.
- If you find a clearly better implementation for something UI-related, you may do it, but keep it low risk and explain it. Anything that touches core logic: stop and flag it, do not just do it.
- Never use things that do not exist. Do not add menu bars, menu items, tabs, buttons, sliders or any control that has no real function behind it. If a setting or field is not exposed by the firmware or the backend, do not show it. Never fake data and never imply features that are not there.

## 1. Important: you have NOT seen the real web UI yet

At the time this brief was written, neither the author nor you have seen the actual web UI implementation in this repo. So you do not yet know the real information architecture: which sections exist, which fields the backend exposes, or how the current pages are organized.

Therefore:

- These guidelines define the visual language, the voice and the constraints. They are NOT the literal page structure.
- Step one is to study the existing web UI and firmware UI in the repo, then map the real sections and fields that actually exist.
- Build the information architecture from what the repo actually exposes, then dress it in the language below. The reference HTML shows the look and the components, not a fixed feature list.

## 2. References (look at these first)

All under `docs/claude/guidelines/`:

- `ui-01.webp` and `ui-01.jpg`: photos of the physical device.
- `ref-device-screens.html`: pixel-accurate target for every device screen at true 200x200.
- `ref-web-config.html`: target for the visual language of the mobile web config (components and look, not the final feature list).

Open the two HTML files (via the screenshot tool or a browser) and treat them as the visual contract for style, mascot strategy, voice and the 200x200 device layout.

## 3. The aesthetic (decided, do not redesign)

Classic Macintosh System 6, pure 1-bit black and white, on both surfaces, so the device and the web app feel like one product.

- Striped title bar with a close box. No fake application menu bar.
- The Bisc8 cookie mascot as the small app glyph in the title bar (about 20 to 24 px).
- Chunky beveled buttons (use a hard offset border or a 2 to 3 px solid offset, never a blurred shadow).
- 50 percent ordered dither for active or filled states. No smooth gradients, no gray fills other than dither.
- The rubberhose cookie mascot is the character. It owns the boot screen and the idle screen full size. On the responso screen it shrinks to the title-bar glyph so the words own the canvas. Never show the big mascot and a long responso at the same time on the 200x200 screen.

## 4. Device screen constraints (200x200, 1-bit)

- Design at 1:1. Snap every element to the pixel grid. No anti-aliased vector text. Bitmap font only.
- Refresh: partial refresh is supported but it ghosts, so a full refresh is required periodically. Use a FULL refresh for the responso reveal and treat that e-ink flash as the oracle's dramatic beat (press, flash, responso). Use partial refresh only for tiny status changes (battery icon). Force a full refresh after a small number of partials so the panel never ghosts.
- Input reality: the board has PWR and BOOT buttons and an LED on EXIO4. Single PWR press is pesca un responso. Long press is sleep, and optionally cycle the active register. There are NO on-device menus. All configuration lives in the web panel.
- Audio: the board has the ES8311 codec, a speaker amplifier and a microphone. Add a "speaking" UI state (a small speaker glyph that animates) for when Bisc8 voices the responso, and leave a clean hook for an optional voice-ask state ("ti ascolto") later. Do not build the audio pipeline unless asked. Build the UI states and the hooks.
- Screens to cover: boot, responso, pesca / thinking, idle / sleep, no Wi-Fi, low battery, first run empty. Copy is in section 6.

## 5. Web config constraints (mobile-first)

- Single column, big tap targets, compact, immediate. It should be understood at a glance with no manual.
- Same System 6 1-bit language as the device. The rubberhose cookie is the header glyph, the loading state and the empty state.
- Surface only the sections and fields that actually exist in the repo. Likely candidates to look for: device name, Wi-Fi, responsi management per register, display contrast, e-ink refresh mode, OTA update, and a pesca di prova preview. Confirm each against the code. Hide anything that does not exist. Do not invent a backend.

## 6. Copy and voice (Netmilk)

Voice: confident, self-ironic, nerd pride, anti-corporate, warm and sagace. Short and punchy on a tiny screen. Never corporate, never generic. Same register as the Synthalia README.

Multilingual: Bisc8 speaks several European languages. Every UI string and every responso must exist per supported language (at least IT, EN, ES). Transcreate the swagger, do not translate literally. The joke has to land in each language.

Typography rule for all written content: no em-dashes and no en-dashes. Use hyphens, commas, colons or parentheses.

Saved phrases to use and extend (keep this voice):

- Pesca / thinking: "Consulto le briciole..." and "Interrogo il cosmo. E il forno."
- Boot: "Bisc8, by Netmilk Studio. Premi PWR, se hai coraggio."
- No Wi-Fi: "Niente Wi-Fi. L'oracolo medita offline."
- Low battery: "Batteria all'osso. Anche la preveggenza si stanca."
- Idle / sleep: "Zzz. Torno se mi schiacci."
- First run empty: "Zero responsi caricati. Che oracolo a digiuno."

The responsi pool already lives in the repo. Keep its tone and do not flatten it.

## 7. Typography (critical: extended Latin)

One shared bitmap or pixel typeface across both surfaces so they match.

It MUST cover extended Latin, at least Latin-1 Supplement (U+00A0 to U+00FF) and Latin Extended-A (U+0100 to U+017F), because the responsi are multilingual. Required glyphs include:

`a` variants: à á â ä, `e`: è é ê ë, `i`: ì í î ï, `o`: ò ó ô ö ø, `u`: ù ú û ü, plus ñ ç ß å æ œ ł ő ű ē š ž č ć đ and their uppercase forms.

- Web: use a pixel font with this coverage. Recommended starting point: Pixelify Sans. Verify glyph coverage. If any required glyph is missing, switch to another pixel face that covers extended Latin. Do NOT silently fall back to a smooth system font.
- Device: use U8g2 (U8g2_for_Adafruit_GFX) with a bitmap font that includes extended Latin (a Unifont based face or a large U8g2 "_tf" / "_te" variant). Pick the one that fits 200 px well, matches the pixel and Mac look, and renders every required diacritic. Two sizes: a headline of about 16 to 20 px for the responso, and a small of about 10 to 12 px for chrome and status.
- The current font may be replaced. Choosing a better covering and better looking one is in scope and encouraged.

## 8. Validation loop (mandatory)

Use the screenshot tool already present in the repo. Do not declare any screen done before its screenshot matches the reference.

Loop: build or adapt the UI, run the screenshot tool, compare against the reference HTML, fix, repeat.

Acceptance criteria, all must pass:

1. Device screens match `docs/claude/guidelines/ref-device-screens.html` at 200x200, 1:1, crisp pixels, no anti-aliased text.
2. Web config uses the visual language of `docs/claude/guidelines/ref-web-config.html` on a viewport around 390 px wide, with only real sections.
3. A diacritics test string renders fully on BOTH surfaces with no missing glyphs and no tofu boxes:
   `àèéìòù çñ üöä ß øåæœ łőűē šžčćđ`
4. The responso reveal uses a full refresh. Status-only updates may use partial refresh. A full refresh is forced after a few partials and there is no visible ghosting.
5. The mascot is full screen on boot and idle, and a 20 to 24 px glyph on the responso screen.
6. Copy is multilingual, in the Netmilk voice, with no em-dashes or en-dashes anywhere.
7. No control or element exists in the UI without a real function behind it.

When all criteria pass on screenshots, summarize what changed and list anything you flagged but did not touch.
