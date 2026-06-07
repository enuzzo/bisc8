# Handoff (next session)

## ▶ Open / next round
- **General design inspection.** A fresh, deep visual pass across all surfaces
  (site, captive portal, on-device screens, email) — catch anything off after
  all the recent churn.
- **Drop the battery ICON on the device, keep just the percentage** (bottom-right
  footer). The "%u%%" is plenty; the little glyph is redundant. Likely in
  `display_service.cpp`: `RenderBattery()` sets `footer_right_` (the % text) and
  shows `batt_icon_group_` — keep the former, stop showing the latter (and check
  every layout that calls `set_hidden(batt_icon_group_, false)`). Needs a reflash
  to verify via SNAP.

---

## Latest session: new logo everywhere, email reskin, hardware-verified screens (2026-06-07)

Brand polish across all four surfaces (site, captive portal, on-device e-paper,
email), plus an on-hardware flash/verify pass. The product is functionally
complete; this was identity + accuracy.

### New cookie-wizard logo
- New brand logo (cookie-headed wizard with a crystal-ball "8" + crescent staff).
  Sources in `assets/logo/`: `bisc8-logo.png` (full-colour master, 3.2MB),
  `bisc8-logo-portal.png` (small, base64'd into the portal), `logo_min_v2.jpg`
  (white-bg, the device boot source), `logo_min.png` (derived 128x128 1-bit).
  The 100-file `assets/fonts/itc-garamond/` kit and `assets/logo/archive/` are
  deliberately NOT committed (commercial font / scratch).
- **Site hero**: logo big on the left, copy on the right (`docs/index.html`).
- **Captive portal**: logo at the top, embedded as `__LOGO_B64__` via
  `tools/embed_portal.py`. Both site + portal: the logo floats and is framed by
  twinkling sparkles (`.spark` s1-s4, butter glow).
- **Device boot logo**: 128x128 1-bit, generated from `logo_min_v2.jpg` (white
  bg) by `tools/generate_logo_assets.py` (SIZE=128). IMPORTANT: the boot source
  MUST be white-bg — the old v2 .png had a grey fill that thresholded to a black
  box. LayoutBoot shows it as big as fits (centred, top padding) with "Bisc8" +
  "by Netmilk Studio" below, NO "press PWR" (it's booting). LayoutLowPower reuses
  the same asset at scale 156 (~78px). If the boot asset size ever changes, the
  low-power scale must be recomputed (78/SIZE*256).

### "Bisc8" branding rule (enforced)
The brand name is ALWAYS "Bisc8", never "BISC8". Web: `.brand{text-transform:none}`
utility wraps the name inside any uppercase UI (buttons, menu, footer). Device:
"Bisc8" is set directly (never via UpperAscii). manifest name = "Bisc8".

### Email full reskin (`server/bisc8-email.php`, deploy-only — host re-uploads)
Same look & feel as the site: rose desktop, cream card, no pure black (#000 ->
#23211c), removed the tiny inline mascot. Site typefaces via @font-face base64
(ChiKareGo2 + Pixolde; serif = Georgia, NOT ITC Garamond — commercial + heavy).
@font-face renders in Apple Mail/iOS/Outlook-Mac; Gmail/Outlook-Win fall back to
monospace/serif. Question + answer are equal-size button-style boxes (cream, no
rose); .wav chips enlarged to web-button proportions; all fonts >=16px (only the
hidden 1px preheader stays).

### On-hardware flash + verify (SNAP framebuffer dump)
- Flashed via `idf.py ... -p /dev/cu.usbmodem14201 flash`. Capture: serial
  `SCREEN <NAME>` (forces a screen) + `SNAP`, decoded by
  `tools/capture_epaper_snapshot.py` (writes 1-bit 200x200 PNG). Boot screen: no
  SCREEN cmd, so reset (`esptool ... chip_id`) + capture with ~4.5s settle.
  SCREEN previews render in ENGLISH (design previews); the SCREEN SPEAK demo
  answer is a hardcoded Italian string ("Si. Ma non dirlo a nessuno.") — only
  shown via the debug cmd, swapped to English on the site.
- BUG FIXED: low-power mascot scale was calibrated for the old asset size and
  overlapped the wake text after the boot logo grew. Now correct.
- All site/README device screens refreshed from REAL captures (English):
  status, reading, low-power, setup. **PRIVACY**: the dev device is connected to
  a home Wi-Fi whose SSID shows on the status screen — it is REDACTED to
  "Home Wi-Fi" in the published `docs/img/screen-status.png`. Never publish the
  real SSID. README points at the same `docs/img` files, so it updates too.

### Misc
- Captive portal status cells <=60px tall, value text 1.5rem. Title chip fills
  the bar height (shared top/bottom edges) on portal + site + modals. 1.1rem
  font floor on portal + site. "The board" hardware section added to the site
  (real ESP32-C6 photo, 2-column spec grid, ~EUR15-20, AliExpress link). Site
  describes the **ESP32-C6** (the real board) — the spec sheet that floated
  around was for the ESP32-S3 variant; don't use it.
- README de-em-dashed (house style: no em-dashes). NOTE: the SITE copy
  (`docs/index.html`) still has a few em-dashes — clean those if asked.

### Handy
- Flash: `export BISC8_IDF_TOOLS_PATH="$PWD/.espressif"; . tools/idf_env.sh;
  python "$IDF_PATH/tools/idf.py" -C firmware/bisc8_fortune -B "$HOME/bisc8-build" flash`
- Capture a screen: `python3 tools/capture_epaper_snapshot.py --port
  /dev/cu.usbmodem14201 --before-command "SCREEN LOWPOWER" --before-delay 3
  --settle 5` (retry on "SNAP length mismatch").
- Tests: `.espressif/python_env/idf5.5_py3.9_env/bin/python -m pytest tests/` (96).
- Site is live at https://enuzzo.github.io/bisc8/ (Pages, main /docs).

---

## Latest session: "Bisc8 OS" revamp shipped — public Pages site + portal typography v5 + model swap (2026-06-06)

A pure design/distribution session. The product is functionally complete; this was
aesthetics + getting it shippable to the public.

### Follow-up fixes (same day, after the revamp)
- **1.1rem font floor everywhere** (portal + site): no text below 1.1rem on either
  surface. Portal v5 sub-1.1 sizes raised + reboot-bar/toast overrides; 27 site
  declarations bumped. (commit ff79942)
- **🩹 Idle e-paper refresh FIXED** — root cause was `StartArrowBlink()` in
  `display_service.cpp`: the home/idle press-arrow ran a 520ms blink timer (10
  ticks), each tick forcing an e-ink partial refresh, and every ~30 partials the
  anti-ghost logic forced a full-screen flash → "screen refreshes by itself while
  idle/low-power". Fix: render the arrow ONCE, statically, NO timer (same
  no-refresh-at-rest rule as the mic screen). `SetBattery()` is just a setter (no
  render), and only button presses post events — so with the timer gone the idle
  home screen is truly static.
- **🔋 ≤10% battery → full auto power-off** (`app_main.cpp`): new
  `kCriticalBatteryShutdownPct = 10` + `battery_is_critical()` + a
  `power_off_if_critical()` guard called at boot (covers wake-from-sleep) AND on
  every event. It writes a dedicated screen (`display.ShowCriticalLowBattery` —
  big battery glyph + localized "powering off" message, new `low_battery_off_body`
  string EN/ES/IT), plays the Shutdown cue, then `EnterDeepSleep("low-battery", …)`.
  (≤12% still just warns at boot, unchanged.) README updated.
- **Firmware builds clean** (idf.py build OK, bin 58% free). ⚠️ Needs a reflash +
  on-hardware verification: (1) confirm the idle screen no longer flashes, (2)
  confirm the ≤10% cutoff fires and shows the message.

### Shipped (done)
- **Repo is PUBLIC + GitHub Pages is LIVE** → **https://enuzzo.github.io/bisc8/**
  (source `main` / `/docs`, first build OK). Done via `gh repo edit --visibility
  public --accept-visibility-change-consequences` and `gh api -X POST
  /repos/enuzzo/bisc8/pages -f 'source[branch]=main' -f 'source[path]=/docs'`.
- **Internal notes moved `docs/ → notes/`** (AI_HANDOFF, HANDOFF_NEXT, PROJECT_STATUS,
  hardware/, redesign/, gpt-suggestions/). `docs/` now contains ONLY the published
  site (index.html, manifest.json, .nojekyll, fonts/, img/, firmware/). Verified on
  the live site: `/AI_HANDOFF.md` etc. return **404** (not published). `.nojekyll`
  can't exclude files, so moving them out was the only way.
- **Pre-public secret scan: CLEAN.** No live `sk-` keys, tokens, real emails, or the
  home Wi-Fi SSID anywhere in tracked files, git history, or the public `.bin`
  (only placeholders `sk-example`, `tu@esempio.it`). Home SSID was earlier redacted
  from `docs/img/screen-status.png`.
- **Models — `gpt-4o` removed everywhere** (deprecated for audio, per user, confirmed
  by voice): STT `whisper-1`, text `gpt-5.4-mini`, TTS `tts-1-hd`. Changed in
  `app_config.cpp` `DefaultOpenAiSettings()` AND the portal placeholders
  (`web_portal.html` → regenerated `web_portal.cpp`) AND the README flow diagram.
  (Voice stays `coral`.) Only remaining "gpt-4o" string is a comment documenting the
  deprecation.
- **Distribution site `docs/index.html`** (Poolsuite + Mac System 7/8 + biscuit
  identity): 4 device screens moved up under the hero (Status/Reading/Low power/Setup),
  full-width Installer window with stroked step-number tiles + a 100%-width FLASH BISC8
  button, About as a 2-column spec grid (title | value bar). esp-web-tools flasher
  wired to `docs/manifest.json` (ESP32-C6, offsets 0/0x8000/0x10000) + the 3 bins.
- **Captive portal typography v5** (`web_portal.html`, the `TYPOGRAPHY v5` block):
  bigger + airier, harmonized rem scale. Key change: hint `line-height 1.1 → 1.42`
  and `font-size 1.2 → 1.3rem` (the cramped 2-line hints the user flagged), wider
  column (`.app` `390px → 26rem`), more group/section/field spacing, all body text
  bumped. **Regenerated `web_portal.cpp` via `tools/embed_portal.py`.**
  ⚠️ **Portal changes need a reflash to show on-device** (HTML is embedded in firmware).
- **Tests realigned to the redesign** and green (**94 passing**): `LayoutWifiSetup →
  LayoutStatusQr`, status body `"%s\n%s"`, QR-based Wi-Fi setup, showcase-README
  phrase list, `notes/AI_HANDOFF.md` path, "GitHub Pages flasher" copy.
- **Type system**: ChiKareGo2 (titles/labels/buttons), Pixolde (body/hints + diacritic
  fallback — covers `ù ú ü ' ñ ç ¿¡` which ChiKareGo2 lacks), ITC Garamond (long-form
  copy on the site), Pixelify Sans (on-device e-paper + email wordmark). Rule kept:
  titles = ChiKareGo2, never Garamond, never all-uppercase; Garamond only for copy
  >3-4 lines.

### Still to do — DEEP VISUAL ASSESSMENT + FONT HARMONIZATION (the user's explicit next ask)
The v5 portal pass was a quick global bump. A proper pass remains:
- **Define ONE shared rem type scale** (e.g. a documented ladder: chip / section-label
  / field-label / body-hint / value / input / button / footnote) and apply it
  consistently across BOTH the captive portal AND `docs/index.html`. Right now sizes
  are tuned per-surface by feel, not from a single scale.
- **Audit every text element** for size/weight/line-height coherence; remove leftover
  one-off px values; make sure the visual hierarchy reads the same on both surfaces.
- **Test the captive portal on a REAL phone** (360–430px), not just the desktop
  preview — the device serves it to a phone browser. Watch for overflow with the new
  bigger type.
- **On-device e-paper screens** (Pixelify Sans @ 200×200, 1-bit): verify legibility of
  the revamped status/QR/voice screens after a reflash.

### Concrete nits found during this session (queue for the deep pass)
- **Portal Voice field placeholder says `alloy`** but the real default voice is
  `coral` (`app_config.cpp`). Align the placeholder.
- **Portal: the muted hint beside the "Scan" button** ("the networks nearby, nothing
  more.") wraps awkwardly next to the button — move it below or shorten.
- **⚠️ ITC Garamond licensing**: it's a COMMERCIAL font, now served publicly via Pages
  (`docs/fonts/ITCGaramondStd-*.woff2`) and present in git history. Mitigate by
  swapping the site's long-form serif to **EB Garamond (OFL, free, near-identical)**,
  or confirm a valid license. The full 100-file source kit `assets/fonts/itc-garamond/`
  was deliberately NOT committed (stays local/untracked).
- **`public/flash/`** is a stale duplicate of the `docs/` flasher (not served by Pages,
  no secrets). Either remove it and repoint `test_public_flash_page_*` at `docs/`, or
  keep it as the build scaffold. Decide.
- **Site footer date** renders e.g. "SAT 6 JUN 2303" — confirm it's intentional
  retro-future flavor vs a JS year bug.
- **Tagline case**: confirm the portal IT tagline is "Briciomanzia Tascabile"
  (first-letter caps, no letter-spacing, 1.3rem ChiKareGo2) across i18n.

### Where things live / handy commands
- Portal preview (fonts inlined): `tools/embed_portal.py` writes `/tmp/portal_preview.html`.
  ALWAYS run `embed_portal.py` after editing `web_portal.html` or the device serves stale HTML.
- Tests: `.espressif/python_env/idf5.5_py3.9_env/bin/python -m pytest tests/` (94).
- Live site: https://enuzzo.github.io/bisc8/ · Pages build status:
  `gh api /repos/enuzzo/bisc8/pages/builds/latest`.

---

## Latest session: dropout fix verified + lyrical/theatrical oracle + styled localized email (2026-06-05 eve)

Ran the voice-sampling test from the previous session **on hardware** and went well
beyond it. Headlines:

- **Clipping fix CONFIRMED** on-device: `clip=0.00%`, `peakMono` mid-scale, clean
  full transcript. The 45→24 dB gain change was correct.
- **Found & fixed a SEPARATE bug: recording dropouts / stutter.** Held BOOT ~10 s
  but only ~⅓ of the audio was saved (`wall` >> `dur`, `maxRead` ~735-776 ms),
  truncating speech into gibberish to STT. Root cause: the **e-paper "listening"
  animation** (a 750 ms `lv_timer` pulsing the mic waves) — each tick fires an
  e-ink PARTIAL REFRESH, and on the **single-core C6** that long SPI/BUSY-wait
  steals the core from `VoiceRecordTask` (prio 4), overflowing the mic DMA (~90 ms).
  Distinguished from a sample-rate bug BY EAR: stutter at normal pitch = CPU
  starvation; chipmunk = clock. **Fix** (`display_service.cpp` `StartVoiceAnim`):
  the mic/listening screen now renders ONCE, statically (both waves shown), with NO
  refresh timer; only the post-capture "thinking" screen keeps its pulse. Verified:
  `dur≈wall`, `maxRead` ~241 ms, clip 0%, full clean transcript. **Rule: never
  refresh the e-ink while the mic is recording.**
- Brain TLS `mbedtls_ssl_setup -0x7F00` (ALLOC_FAILED) on the chat step was a
  **transient** heap pinch — succeeded on retry. Watch heap if it recurs.

Product/voice tuning (all shipped):
- **Playback volume** `kAnswerGainPercent` 150 → **180** (`audio_service.cpp`).
- **Lyrical answers**: brain system prompt pushed to "lyrical, incantatory, musical,
  cadence of verse, vivid sensory detail" while staying pertinent (was deliberately
  anti-cryptic). **Theatrical voice**: TTS `instructions` now "theatrical seer in
  the throes of a vision" (rising/falling, emphasis). `voice_oracle_service.cpp`.
- **Translation audit** (`localization.cpp`): `cooking_title` was untranslated
  ("Cooking") in ES/IT → "Cocinando"/"Cucino"; the new playful listening copy added
  to EN/ES ("Bisc8 is listening. Max 15s, no rambling!" / "Bisc8 te escucha. ¡Máx
  15s, sin rollos!"); ES `mantén` accent.
- **Web portal** (`web_portal.html`): added a header tagline (`tagline` i18n key:
  IT "briciomanzia tascabile" / EN "pocket crumb-oracle" / ES "migamancia de
  bolsillo") and fixed two leaks where the Italian word **"responsi"** sat inside
  EN/ES copy (`lang_hint`, `send_resp`). NOTE: portal HTML is embedded in firmware,
  so it needs a reflash to take effect.

**Styled, localized email** (`server/bisc8-email.php`, deploy-only, host re-uploads):
- HTML "biscuit-terminal" look matching the web app (black/white, 2px borders, hard
  shadow, Pixelify Sans via Google Fonts with **monospace fallback** for Gmail/Outlook,
  black title bar, inverted RESPONSO hero, audio pills). MIME is now
  `multipart/mixed → alternative(plain+HTML) + WAVs`; plain text kept as fallback.
- **Localized** by the detected language: subject, labels, AND attachment filenames
  (`domanda/risposta.wav` · `pregunta/respuesta.wav` · `question/answer.wav`).
- **Pastry-divination theme**: badge BRICIOMANZIA / CRUMBOMANCY / MIGAMANCIA, tagline
  "divinazione tra le briciole", response label "Letto nelle briciole", footer
  "Sbriciolato per te da Bisc8".
- **Full localized date/time** ("Giovedì 5 giugno 2026, 18:42", tz default
  `Europe/Rome`, override via config `timezone`), 🍪 in the subject, and a hidden
  **preheader** for the inbox snippet.

Build note (IMPORTANT): **this flash Mac is Intel x86_64**, not Apple Silicon.
`arch -arm64` fails here. Let `tools/idf_env.sh` auto-pick the **x86_64 / py3.9**
env (builds & flashes fine). Do NOT apply this doc's older "use the arm64 / py3.14
env" advice on this box — that was for a different, arm64 Mac.

Email relay troubleshooting (learned the hard way): the device POSTs `to` (recipient)
+ `token` (Bearer header AND form field); the PHP health-check `ready` flag needs
BOTH config `token` and `mail_to` non-empty, but `mail_to` may legitimately be empty
(recipient comes from the device) — so `ready:false` does NOT prove the token is
wrong. Diagnose with `curl` GET (health) then POST with the token. The actual fault
this session: the **device app had lost its email/relay/token settings**; the host
config + token were fine (proven by a `curl` POST that delivered). Relay token must
be byte-identical on host config and the captive portal.

Open / nice-to-have:
- **Mic right channel is dead** (`peakR`≈185 vs `peakL`≈3824); we downmix `(L+R)/2`,
  so we throw away ~6 dB → `rms` ran ~-26..-42 dBFS (quiet but STT-OK). Canonical
  fix: capture the LEFT channel only instead of averaging (see the ES8311 ref note
  below). Optional gain bump if still quiet.

---

## Previous session: capture audio quality fix + answer-audio email (2026-06-05 pm)

**Goal:** the user's recorded question reached speech-to-text as gibberish (the
emailed `domanda.wav` sounded terrible), while the early record->playback
loopback was excellent.

**Root cause (high confidence): mic clipping.** `Codec_StartInit` set the ES8311
`in_gain` to **45.0 dB**, but the chip's mic PGA tops out at **42 dB**
(`es8311_set_mic_gain` buckets anything >=42 to the max), so the mic ran at
MAXIMUM analog gain. On this near-field pocket device that rails the ADC on
ordinary speech -> hard clipping -> STT hears phonetically-rich gibberish. The
ear (and the speaker loopback) tolerate clipping that STT does not, which is why
the SAME mic sounded "ottimo" in the loopback yet transcribed as nonsense, and
why it was intermittent (level-dependent): the emailed transcripts alternate
clean ("Pioverà domani?") and gibberish ("Manitroona trimulindusperdu") minutes
apart -- not a uniform sample-rate error, and never empty (loud-distorted, not
weak/far).

**Fix:** `port_codec.cpp` `in_gain` 45.0 -> **24.0 dB** (strong but unclipped for
close talk, with headroom). One change, one variable -- to be confirmed by
measurement on-device (below).

**Reference check (Waveshare/ES8311 user guide, as the user suggested):** the
ES8311 *analog* mic preamp is **0-30 dB**; 36/42 dB are *digital* boost (more
noise, harder clipping), and the codec/driver default is **42 dB (max)** -- so
45.0 was the worst case. 24 dB is squarely in the clean analog band, which is why
the tune window is 18-30. The reference record config also uses **mono, left
slot** (`I2S_SLOT_MODE_MONO`/`I2S_STD_SLOT_LEFT`), not stereo + `(L+R)/2`; our
codec is opened 2-ch and downmixed. That's fine today (clean transcripts prove
the format works), but if `[VOICEDIAG]` shows `peakR` ~ 0 the canonical move is to
capture a single channel instead of averaging.

**Also shipped this session:**
- **Capture diagnostics** (`audio_service.cpp`): one `[VOICEDIAG]` serial line per
  recording -- per-channel + mono peak, RMS dBFS, clip%, worst codec-read /
  flash-write stall (ms), and wall-clock vs frame-derived duration. This is the
  test harness: it names the defect instead of guessing.
- **`tools/analyze_question_wav.py`**: desktop twin of `[VOICEDIAG]` -- run it on
  any emailed `domanda.wav` to get clip%/dBFS/dropout/per-channel verdict
  (CLIPPING vs WEAK/FAR vs DROPOUTS vs levels-OK). Stdlib only.
- **Answer audio in the email** (`email_service.*`, `bisc8-email.php`,
  `app_main.cpp`): the verification email now carries transcript + answer text +
  `domanda.wav` (question) + `risposta.wav` (the TTS answer) -- text and BOTH
  voice samples per query. The TTS WAV's OpenAI "unknown length" (0xFFFFFFFF)
  header is repaired to a real length so desktop players open it.

**>>> DONE — the voice sampling test ran and PASSED (clipping fixed); it also
surfaced & fixed a separate dropout bug. See "Latest session" at the top. <<<**
1. **Build on the machine you flash from** -- `$HOME/bisc8-build` is LOCAL (not
   Dropbox-synced), so it won't exist on a different Mac; rebuild it. Set the env
   (see Build/flash below), then build, then plug the device in, find the port
   (`ls /dev/cu.usbmodem*` -- it re-enumerates on replug), and flash + monitor:
   `idf.py -C firmware/bisc8_fortune -B "$HOME/bisc8-build" build` then
   `idf.py -C firmware/bisc8_fortune -B "$HOME/bisc8-build" -p <PORT> flash monitor`.
2. Hold BOOT, speak a clear phrase, release. Read the `[VOICEDIAG]` line:
   - `clip=` should now be ~0% and `peakMono` well under 32000 (was the bug if high).
   - `rms_dBFS` healthy ≈ -20..-12. If `< -35` it's too quiet -> raise gain
     (`port_codec.cpp` 24 -> 30). If `clip` still > 0.5% -> still hot -> lower (24 -> 18).
   - `peakL` vs `peakR`: if one is ~0, the codec gives mono in a single slot ->
     change the downmix to take that channel instead of `(L+R)/2`.
   - `wall` ≈ `dur` (and small `maxRead`/`maxWrite`) confirms NO stalls -> rules
     out dropouts. If `wall` >> `dur` or the stall times spike, revisit decoupling
     the I2S read from the flash write.
3. The email now has `domanda.wav` + `risposta.wav`. Run
   `python3 tools/analyze_question_wav.py <domanda.wav>` -> expect "levels look OK",
   clip ~0, and a duration matching how long you actually spoke (mismatch => rate).
4. Confirm the transcript matches what you said. Repeat 2-3 phrases at different
   volume/distance to dial in the best gain, then lock it in.

---

Last session: the **online GPT voice oracle**, end to end, plus a lot of polish.
Date: 2026-06-05.

## State: working on hardware

Hold BOOT, speak a question, release: Bisc8 records, transcribes, asks the LLM,
speaks the answer aloud (coral voice), shows it on screen, and (optionally) emails
the transcript + answer + the original recording. Confirmed working by the user.

Host tests: `94 passed` (on this Intel box:
`.espressif/python_env/idf5.5_py3.9_env/bin/python -m pytest tests/` — or any
python3 that has pytest; the in-tree envs lack it).

## The voice oracle pipeline

`VoiceOracleService::AskFromRecordedAudio` (`main/voice_oracle_service.cpp`):
1. **STT** -> `POST /v1/audio/transcriptions`, multipart, the WAV streamed straight
   from the `spool` flash partition (offset 0), model `gpt-4o-mini-transcribe`.
2. **Brain** -> `POST /v1/chat/completions`, model from settings, returns JSON
   (`detected_language`, `oracle_answer_screen`, `oracle_answer_full`, `tts_text`,
   `voice_direction`). `response_format=json_object`, `max_completion_tokens`,
   `reasoning_effort` sent only when set (else `temperature`).
3. **TTS** -> `POST /v1/audio/speech` (wav), streamed to the answer spool region.
   `instructions` = a pinned "warm mystical seer" style + the per-answer
   `voice_direction`. Voice defaults to `coral`.
4. Playback: `AudioService::PlayAnswerAudio` reads the answer WAV from flash.

### Non-obvious things (read before touching audio/config)

- **The flow runs on a dedicated 16 KB-stack worker** (`RunOracleOnWorker` /
  `RunEmailOnWorker` in `app_main.cpp`). The main task has only 3584 B; the mbedTLS
  handshake overflows it (stack-protection panic). Any new TLS call must run on a
  worker too.
- **The TTS is 24 kHz but plays at 16 kHz.** The ES8311 is opened in TDM mode and
  reopening the codec to 24 kHz does NOT retune the I2S clock (it would play 1.5x
  slow + an octave low). `PlayAnswerAudio` therefore **resamples 24->16 kHz**
  (exact 3:2, integer) and keeps the codec native. Side effect: some HF loss
  ("muffled") because the 16 kHz Nyquist is 8 kHz. A real fix is making the codec
  actually run at 24 kHz.
- **OpenAI streams the answer WAV with a 0xFFFFFFFF "unknown length" data size.**
  Playback trusts `AnswerAudioBytes()` (bytes actually written), not the header.
- Quiet TTS is lifted by `kAnswerGainPercent` (currently 150, clipped). Tunable.
- **Question spool is pre-erased at idle** (`RearmQuestionSpool`, at boot and after
  each query) so recording starts instantly; the 512 KB erase otherwise cost ~1-2 s
  at press time. The voice record buffer is a small 16 KB chunk (`kVoiceChunkBytes`)
  so it allocates even on a fragmented heap (64 KB failed).

### Config gotcha that bit us (reasoning_effort -> 400)

`ConfigStore::Load` = `ApplyDefaults` then per-field NVS overlay. A NEW
`OpenAiSettings` field gets the ApplyDefaults value, while the model fields come
from older NVS. So defaulting `reasoning_effort="low"` sent it to a user whose
saved model was `gpt-4o-mini` (which rejects it) -> HTTP 400 -> error screen. Now
`reasoning_effort` defaults empty and is portal-configurable. Add any new
model-coupled param the same way (empty default + portal field).

## Error codes (error-screen footer, "cod. EXX")

E01 recording failed, E02 no API key, E03 STT/network, E04 nothing heard (empty
transcript), E05 Brain. Ask the user to read the code; STT/Brain error bodies are
now logged (`[ORACLE] brain http status=.. body=..`).

## Models / voice / language (portal -> OpenAI section)

- All models are portal fields (`transcription_model`, `response_model`,
  `speech_model`); voice is a dropdown; `reasoning_effort` is a dropdown.
  Defaults: `gpt-4o-mini` / `gpt-4o-mini-tts` (proven), voice `coral`, reasoning off.
- Placeholders SUGGEST `gpt-5.4-mini` / `gpt-realtime-1.5` (user's choice) but these
  are **unverified** by us. `gpt-realtime-1.5` may be a Realtime-API model that does
  NOT work via `/v1/audio/speech` -> would E05. If the user sets them and it fails,
  read the logged body and adjust the name/params or rearchitect TTS to the Realtime
  API. The user's NVS still holds `gpt-4o-mini`; they must TYPE a new model to switch.
- Reply language follows the **transcript** (the words actually spoken, read by the
  model), not the STT language tag (which mislabels short speech). UI/device language
  is just the menus. User confirmed: reply follows what they speak.

## Email (works)

`EmailService::SendOracleEmail` POSTs multipart (token + transcript + answer text
+ the question WAV `audio` field + the answer WAV `answer_audio` field) to a
user-set `relay_url`. The relay is a **standalone PHP file we own**:
`server/bisc8-email.php` (+ `.config.example.php`, README). No email credentials
on the device, only the relay token. The user deploys it to their own host
(currently `https://netmi.lk/nomenomen/api/bisc8-email.php`) and it sends via the
host's `mail()`. First mail from a new domain often lands in spam.

**REDEPLOY the relay** to get `risposta.wav`: the answer-audio attachment needs
the updated `server/bisc8-email.php` on the host (it now reads the `answer_audio`
upload and attaches it). The firmware sends it regardless; an old relay just drops
it. The answer file field is `answer_audio` (not `answer`, which is the answer
TEXT field).

## UI added this session

- Listening screen: animated **mic glyph** + "Parla ora" (`BuildMic`,
  `StartVoiceAnim(true)`). No full refresh (it looked like a reset).
- Thinking screen: **"Consulto le briciole.."** + animated dots (`BuildWaitDots`,
  `StartVoiceAnim(false)`).
- A start-recording cue (`AudioCue::VoiceStart`, `start-talking.ogg`) plays at the
  true capture start (after the pre-erase), so it is a clear "go".
- New serial previews: `SCREEN ERROR|LISTENING|THINKING`.

## Pending / follow-ups

- **Confirm the mic-gain fix on-device** (see the sampling test above) and lock in
  the best value from `[VOICEDIAG]`. If it needs frequent tuning, consider making
  `in_gain` a portal field (like the model fields) so it's adjustable without a
  reflash.
- **Redeploy `server/bisc8-email.php`** on the host so the `risposta.wav` answer
  attachment actually goes out (the firmware already sends it).
- `docs/AI_HANDOFF.md` is **stale** (still says the OpenAI transport is not
  implemented / returns `ESP_ERR_NOT_FINISHED`); update it + its test assertions.
- Verify `gpt-5.4-mini` / `gpt-realtime-1.5` exist for the user's key.
- Voice quality at 16 kHz is "muffled"; revisit the 24 kHz codec path if it matters.
  (This is the ANSWER playback path, separate from the capture clipping fix.)
- `screenshots/epaper/` is git-ignored (dev SNAPs).

## Build / flash (full incantation -- self-contained)

The toolchain (`.esp/`, `.espressif/`) is in the repo (Dropbox-synced) and ships
**both** a `idf5.5_py3.9_env` (x86_64) and a `idf5.5_py3.14_env` (arm64). Just set
`BISC8_IDF_TOOLS_PATH` and let `tools/idf_env.sh` **auto-pick the env by `uname -m`**.
From the repo root:

```sh
export BISC8_IDF_TOOLS_PATH="$PWD/.espressif"   # toolchain lives in-repo
. tools/idf_env.sh                              # auto-picks py3.9 (x86_64) or py3.14 (arm64)
idf.py -C firmware/bisc8_fortune -B "$HOME/bisc8-build" build
ls /dev/cu.usbmodem*    # find the port (re-enumerates on replug)
idf.py -C firmware/bisc8_fortune -B "$HOME/bisc8-build" -p <PORT> flash monitor
```

Critical gotchas:
- **Build OUTSIDE Dropbox** (`-B "$HOME/bisc8-build"`, local SSD): the in-tree
  `build/` crawls under the FUSE sync layer and is stale anyway. This dir is local,
  so on a fresh machine the first build runs from scratch (a few minutes).
- **Match the env to the host arch — do NOT force one.** `idf_env.sh` picks it from
  `uname -m`. **The current flash Mac is Intel x86_64** → it uses `idf5.5_py3.9_env`
  (confirmed building & flashing fine). Do NOT export
  `BISC8_IDF_PYTHON_ENV_PATH=.../idf5.5_py3.14_env` here: the arm64 env has no working
  `python` on this box (`env: python: No such file or directory`), and `arch -arm64`
  fails (Intel hardware). On an Apple-Silicon Mac the script auto-picks py3.14
  instead — also fine. The rule: let the script choose.
- Host tests: `python3 -m pytest tests/` (any python3 with pytest; the in-tree env
  lacks it).
- SNAP a screen: `tools/capture_epaper_snapshot.py --before-command "SCREEN ..."`.
