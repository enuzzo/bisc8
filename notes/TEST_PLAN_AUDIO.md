# Audio QA bench plan — Monday session (device back in hand)

Goal: in ~15 minutes, prove (a) the mic capture is healthy on THIS unit, (b)
the emailed audio is now listenable, (c) the Wi-Fi join fix holds on real
routers. Everything below was built 2026-06-12 while the device was away;
firmware compiles and all 126 host tests pass, but nothing has run on hardware
yet.

## What changed (context for the session)

- **Email audio regression root-caused**: the "compact audio" commit emailed
  4 kHz WAVs produced by every-4th-sample decimation with NO anti-alias
  filter — 2–8 kHz speech energy folded into the passband (garble), on top of
  the muffle and the 6 s/10 s truncation. Mic capture and STT were never
  degraded: they always used the full 16 kHz spool WAV.
- **Fix shipped (untested on HW)**: `email_service.cpp` now box-filters
  (averages) down to 8 kHz PCM16, question ≤8 s / answer ≤12 s, retry ladder
  renamed `compact8k` → `question8k` → `text`. WAV header carries the true
  output rate.
- **New portal endpoints**: `GET /api/audio/question.wav` (live region, falls
  back to the new archive slot at `0x100000` after the post-flow rearm) and
  `GET /api/audio/answer.wav`. Portal page grew an "Audio" group with both
  download buttons; works in setup mode AND on the home LAN.
- **Wi-Fi**: the 3 s stall-kick now stops once `WIFI_EVENT_STA_CONNECTED`
  fires (it used to abort slow DHCP forever — prime suspect for the friend's
  TIM router); per-network budget 10 s → 12 s.
- **New tool**: `tools/audio_qa.py` (fetch + analyze + grade + email-encoder
  simulator). `tests/test_audio_qa.py` pins it to the firmware math.

## 0. Flash + smoke (5 min)

```bash
export BISC8_IDF_TOOLS_PATH="$PWD/.espressif"
. tools/idf_env.sh
idf.py -C firmware/bisc8_fortune build
idf.py -C firmware/bisc8_fortune -p /dev/cu.usbmodem14201 flash
cat /dev/cu.usbmodem14201 > /tmp/bisc8.log &   # keep serial captured ALL session
```

- [ ] `[BUILD]` line shows today's `r<count>-<date>` version (also in portal header).
- [ ] Boot reaches idle; no new errors vs the previous build.

## 1. Wi-Fi join check (3 min)

- [ ] Boot with the saved home network: log shows `associated; waiting for DHCP`
      then `got sta ip=...` — and NO `reconnect kick ... reason=stall` lines
      AFTER the `associated` line. (Kicks before association are fine/expected.)
- [ ] Optional regression probe: wrong-password network → still falls to the
      setup portal in ≤ ~25 s with `non raggiungo <ssid>` on the e-ink.

## 2. Mic + voice loop (5 min)

1. Hold BOOT, ask a test question at normal distance (~30 cm), release.
2. Watch serial for the `[VOICEDIAG]` line (peaks/duration) — copy it into the
   session notes.
3. After the answer plays, from a laptop on the same LAN:

```bash
python3 tools/audio_qa.py --device <device-ip> --play
```

- [ ] `bisc8-question.wav` GRADE = PASS (no CLIPPING/DROPOUTS/MUFFLED, duration
      matches how long you spoke).
- [ ] `bisc8-answer.wav` GRADE = PASS and sounds like the spoken answer.
- [ ] Repeat once at "friend distance" (~1 m, normal room) — still PASS.

Failure signatures (from analyze_question_wav):
- CLIPPING → lower mic `in_gain`; WEAK/FAR → raise it.
- DROPOUTS / stutter → something refreshed the e-ink during capture
  (see memory: e-paper starvation) or flash stalls — check what drew on screen.
- MUFFLED → mic hole blocked / wrong codec slot.
- Duration mismatch → real capture rate ≠ header rate.

## 3. Email loop (3 min)

1. With email enabled, ask one question; wait for `[EMAIL] relay payload
   mode=compact8k ... status=200`.
2. Open the email on the phone AND on desktop.

- [ ] Both attachments play; question is intelligible telephone-quality (no
      garble/grit), answer likewise.
- [ ] Attachment properties say 8000 Hz (not 4000).
- [ ] Sizes sane: question ≤ ~128 KB, answer ≤ ~192 KB.
- [ ] Edge: ask a >8 s question — email copy truncates at 8 s but STT/answer
      still use the full recording.

Local preview (no device needed) of the encoder, for reference or regression:

```bash
python3 tools/audio_qa.py --simulate-email some_recording.wav   # writes -old4k / -naive / -box
```

## 4. Portal UX pass (2 min)

- [ ] "Audio" group shows two buttons; both download (question served from the
      archive slot even minutes after the flow).
- [ ] Wi-Fi save → bottom bar now says "Wi-Fi salvata. Riavvia per provarla…"
      (no more false "Testata").
- [ ] Status grid, language switch, reboot bar all behave as before.

## 5. If everything passes

- [ ] `tools/prepare_web_flash.py` + publish, so the friend's unit can be
      updated from the web flasher next time it's on USB.
- [ ] Send the friend a fresh oracle email and ask "meglio ora?" — her ears
      are the final acceptance test.
- [ ] Log results in PROJECT_STATUS.md; close the audio-regression item.
