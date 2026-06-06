# Bisc8, OpenAI Voice Oracle Integration Handoff

Nota per Codex: il repository potrebbe essere già più avanti di questo documento. Prima di implementare, verifica lo stato reale del branch, i file esistenti, il response contract già definito e le API OpenAI aggiornate. Non creare un secondo stack parallelo se esiste già un client o un servizio simile. L'obiettivo è provare a implementare, non fare una riscrittura totale.

## 1. Obiettivo

Implementare il flusso online del Voice Oracle di Bisc8:

```text
BOOT hold
  -> registra domanda utente in WAV
  -> speech-to-text
  -> modello testuale genera risposta breve + risposta parlata
  -> modello vocale legge la risposta con mood Bisc8
  -> e-paper mostra short answer <= 100 caratteri
  -> Bisc8 riproduce audio generato
  -> eventuale email relay invia transcript, full answer e audio se fattibile
```

Bisc8 deve sembrare un piccolo oracolo da tasca, buffo, mistico, curioso e un po' bizzarro. Non deve sembrare un assistente vocale generico.

## 2. Stato atteso del progetto, da verificare nel repo

Dal README risultano già presenti questi pezzi:

- Firmware ESP-IDF per Waveshare ESP32-C6-ePaper-1.54.
- Voice flow UI e stati localizzati già impostati.
- Registrazione audio attivata da hold BOOT.
- Rilascio BOOT o limite 15 secondi per chiudere la registrazione.
- Registrazione salvata come WAV 16 kHz mono su spool flash, attualmente `spool://question.wav`.
- NVS con lingua, Wi-Fi, OpenAI settings, email recipient e relay settings.
- Offline fallback con fortune locali.
- Runtime playback attuale basato su PCM 16 kHz, 16-bit, stereo per evitare decoder OGG/MP3.
- `VoiceOracleService::AskFromRecordedAudio()` oggi dovrebbe essere il punto da completare, ma va verificato perché il branch potrebbe essere già andato avanti.

Non dare per scontati nomi di file e classi. Cerca nel repo prima:

```sh
rg "VoiceOracleService|AskFromRecordedAudio|spool://question|OpenAI|VOICE START|VOICE STOP|audio|I2S|sound_assets"
```

## 3. Scelta architetturale consigliata

### V1: implementazione request-based, non Realtime end-to-end

Per il prodotto attuale, il flusso è domanda registrata -> risposta -> playback. Non è una conversazione live full duplex. Quindi la prima implementazione dovrebbe essere request-based.

Fare così:

```text
Firmware
  - registra domanda
  - carica WAV a OpenAI o a un relay
  - riceve JSON risposta + audio generato
  - salva audio generato in spool
  - riproduce audio

Modelli OpenAI
  - STT per trascrizione
  - modello testuale veloce per la risposta
  - modello audio/TTS per lettura
```

Realtime può essere testato dopo come voice actor, ma idealmente dietro un relay/server, non direttamente sull'ESP32-C6 nella prima versione. Il Realtime API è più vivo e probabilmente più adatto alla recitazione, ma porta sessioni, WebSocket/WebRTC, eventi e complessità inutile per una v1.

## 4. Strategia modelli

Verifica sempre model IDs, endpoint e parametri nelle docs OpenAI attuali prima di committare.

### 4.1 Speech-to-text

Default consigliato:

```text
gpt-4o-mini-transcribe
```

Fallback qualità:

```text
gpt-4o-transcribe
```

Endpoint previsto, da verificare:

```text
POST /v1/audio/transcriptions
```

Input: `spool://question.wav`, upload multipart/form-data. Non leggere tutto in RAM se evitabile.

### 4.2 Brain, cioè la risposta intelligente

Default consigliato per latenza e costo:

```text
gpt-5.4-mini
```

Fallback high quality configurabile:

```text
gpt-5.5
```

Endpoint consigliato:

```text
POST /v1/responses
```

Impostazioni da provare, verificando supporto reale del modello:

```json
{
  "model": "gpt-5.4-mini",
  "reasoning": { "effort": "none" },
  "max_output_tokens": 180
}
```

Se `reasoning.effort: none` non è supportato dal modello scelto, usare il valore minimo supportato. Qui la priorità è la latenza, non un trattato metafisico in sette volumi.

### 4.3 Voice, cioè la lettura con mood

Implementare una piccola astrazione `VoiceProvider`, così possiamo cambiare provider senza riscrivere `VoiceOracleService`.

Provider consigliati:

```text
1. audio_chat
   Modello: gpt-audio-1.5
   Uso: request-based audio output, probabilmente la strada migliore da testare per Bisc8.

2. speech_tts
   Modello: gpt-4o-mini-tts
   Uso: fallback semplice via /v1/audio/speech.
   Nota: più facile, ma probabilmente più piatto, soprattutto in italiano.

3. realtime_proxy
   Modello: gpt-realtime-2
   Uso: esperimento per voice acting più vivo.
   Nota: meglio dietro relay/server, non direttamente su firmware in v1.
```

Configurazione suggerita:

```c
// pseudo config
#define BISC8_STT_MODEL_DEFAULT      "gpt-4o-mini-transcribe"
#define BISC8_BRAIN_MODEL_DEFAULT    "gpt-5.4-mini"
#define BISC8_TTS_MODEL_DEFAULT      "gpt-audio-1.5"
#define BISC8_TTS_FALLBACK_MODEL     "gpt-4o-mini-tts"
```

Se il repo ha già menuconfig/NVS per i modelli, estendere quelli. Non duplicare.

## 5. Response contract del Brain

Il modello testuale deve restituire JSON compatto, stabile e facile da parsare.

Allinearsi al response contract già esistente nel repo. Se manca o è ancora morbido, usare questo schema:

```json
{
  "language": "it",
  "transcript": "Domanda trascritta dell'utente",
  "screen_answer": "Sì, ma aspetta il secondo segno.",
  "full_answer": "Mmm. Le briciole hanno discusso. Sì, ma aspetta il secondo segno.",
  "spoken_lines": [
    "Mmm.",
    "Le briciole hanno discusso.",
    "Sì, ma aspetta il secondo segno."
  ],
  "voice_direction": "Almost whispered, mystical, warm, intimate, dryly funny, with suspenseful pauses. Not horror, not childish, not cartoonish."
}
```

Regole:

- `screen_answer` massimo 100 caratteri, spazi inclusi.
- `screen_answer` deve essere leggibile sul 200x200 e-paper.
- `full_answer` massimo 1-3 frasi brevi.
- `spoken_lines` massimo 3-4 righe.
- Lingua risposta = lingua rilevata nella domanda, salvo fallback.
- Niente markdown nel JSON.
- Niente virgolette “creative” se complicano il parser.
- In caso di JSON non valido: fallback locale e log seriale chiaro.

## 6. Prompt per il Brain

Usare un system prompt simile, poi adattarlo al contract esistente.

```text
You are Bisc8, a tiny black-and-white pocket oracle shaped like a cheeky biscuit.
Answer in the same language as the user.

Personality:
- mystical but not solemn
- funny but not random
- curious, compact, oddly wise
- never corporate
- never motivational-poster
- a little theatrical, like a fortune cookie that has seen too much

Task:
Given the user's transcript, produce a short oracle answer.
Return valid compact JSON only.

JSON fields:
- language: ISO-like language code, e.g. it, en, es
- screen_answer: max 100 characters, suitable for a 200x200 e-paper screen
- full_answer: 1-3 short sentences, suitable for voice playback
- spoken_lines: array of 1-4 short strings for voice playback
- voice_direction: short English voice direction for the TTS/voice actor

Rules:
- Same language as the user.
- Do not mention that you are an AI.
- Do not give medical, legal, financial or safety-critical advice as certainty.
- Be playful, but not nonsense.
- Be concise.
```

User prompt:

```text
Transcript:
{{TRANSCRIPT}}

Device language setting:
{{DEVICE_LANGUAGE}}

Return only JSON.
```

## 7. Prompt per il Voice Actor

Non chiedere al modello vocale di inventare. Deve leggere uno script già scritto.

Voice actor instructions:

```text
You are only the voice actor for Bisc8.
Do not answer the user.
Do not rewrite, translate, explain, or add words.
Read exactly the provided script.

Voice:
- almost whispered, intimate, close to the microphone
- mystical but warm
- dryly funny, with a tiny smile in the last line
- slow enough to feel prophetic
- add natural suspense with pauses
- not horror
- not childish
- not cartoonish
- no singing
- no sound effects
- no extra onomatopoeia
```

Script construction:

```text
<SCRIPT>
Mmm.
Le briciole hanno discusso.
Sì, ma aspetta il secondo segno.
</SCRIPT>
```

Se il provider non rispetta bene le pause, non insistere solo col prompt. Implementare in seguito audio stitching server-side o relay-side:

```text
line 1
+ 350 ms silence
+ line 2
+ 500 ms silence
+ line 3
```

Su firmware diretto, per v1, può bastare testo con righe separate e punteggiatura forte.

## 8. Audio format

V1 consigliata:

```text
WAV, PCM lineare, no MP3, no Opus, no AAC.
```

Motivo:

- WAV è grosso, ma semplice da validare e riprodurre.
- PCM raw è ancora più diretto, ma non ha header e quindi è più fragile.
- MP3/Opus/AAC richiedono decoder e aumentano rischio su ESP32-C6.
- Il progetto oggi evita già decoder compressi per i sound asset.

Implementare playback generato così:

1. Ricevere audio come WAV se possibile.
2. Salvare in spool, es. `spool://answer.wav`.
3. Parsare header WAV.
4. Accettare solo PCM 16-bit.
5. Se mono e il driver vuole stereo, duplicare il campione L/R on the fly.
6. Se sample rate è supportato dal driver I2S, riconfigurare I2S.
7. Se sample rate non è supportato, fallire pulito o usare relay per conversione.
8. Non caricare tutto l'audio in RAM.

Target ideale per il firmware:

```text
16 kHz, 16-bit, mono o stereo PCM dentro WAV
```

Se il provider ritorna 24 kHz, prima provare a riconfigurare I2S a 24 kHz. Se l'audio stack del repo è rigido a 16 kHz, non scrivere subito un resampler brutto: meglio aggiungere una conversione nel relay o nel tool di test.

## 9. Implementazione firmware, ordine suggerito

### Step 0, audit repo

- Trovare `VoiceOracleService` e response contract.
- Trovare spool API.
- Trovare HTTP/TLS helper esistenti.
- Trovare audio playback/I2S API.
- Trovare NVS OpenAI settings.
- Trovare UI states del voice flow.
- Capire se esiste già un client OpenAI parziale.

### Step 1, configurazione modelli

Aggiungere o verificare configurazione per:

```text
openai_api_key
stt_model
default_brain_model
voice_provider
voice_model
voice_name
voice_response_format
```

Default:

```text
stt_model = gpt-4o-mini-transcribe
brain_model = gpt-5.4-mini
voice_provider = audio_chat
voice_model = gpt-audio-1.5
voice_fallback_model = gpt-4o-mini-tts
voice_response_format = wav
```

Se `audio_chat` non è rapido da integrare, implementare prima `speech_tts` ma lasciare `VoiceProvider` pronto per `audio_chat`.

### Step 2, OpenAI STT

Implementare:

```cpp
esp_err_t OpenAIClient::TranscribeWavFromSpool(
    const char* spool_uri,
    std::string* transcript,
    std::string* detected_language);
```

Note:

- Upload multipart/form-data.
- Streaming da spool, non RAM.
- Timeout ragionevole.
- Nessuna API key nei log.
- Loggare dimensione file, durata, status HTTP, tempo totale.

### Step 3, Brain response

Implementare:

```cpp
esp_err_t OpenAIClient::GenerateOracleAnswer(
    const std::string& transcript,
    const std::string& device_language,
    VoiceOracleResponse* out);
```

Parsing robusto:

- JSON valido.
- `screen_answer` troncata in modo sicuro a 100 caratteri, idealmente rispettando UTF-8.
- Se `spoken_lines` manca, derivarla da `full_answer`.
- Se `voice_direction` manca, usare default.
- Se lingua mancante, usare device language o STT language.

### Step 4, Voice audio generation

Interfaccia:

```cpp
class VoiceProvider {
 public:
  virtual esp_err_t GenerateSpeech(
      const VoiceOracleResponse& response,
      const char* output_spool_uri) = 0;
};
```

Provider 1, se fattibile subito:

```text
AudioChatVoiceProvider
model = gpt-audio-1.5
format = wav
input = script + voice actor instructions
```

Provider 2, fallback:

```text
SpeechTTSVoiceProvider
model = gpt-4o-mini-tts
format = wav
voice = configurabile, provare fable/sage/onyx/coral
instructions = voice actor instructions
input = script
```

Provider 3, esperimento successivo:

```text
RealtimeProxyVoiceProvider
model = gpt-realtime-2
implemented on relay/server, not firmware direct
input = exact script
output = wav converted to firmware-friendly format
```

### Step 5, generated audio playback

Implementare:

```cpp
esp_err_t AudioPlayer::PlayWavFromSpool(const char* spool_uri);
```

Se esiste già qualcosa di simile, estenderlo.

Requisiti:

- Streaming da flash spool.
- Header WAV parser piccolo e testabile.
- Supporto PCM 16-bit.
- Supporto mono -> stereo duplication se necessario.
- Reconfig I2S se sample rate differente e supportato.
- Errore pulito su formato non supportato.

### Step 6, UI flow

Integrare in `VoiceOracleService::AskFromRecordedAudio()`:

```text
show Cooking
play voice_submit cue
STT
Brain
show screen_answer
TTS/audio generation
play generated audio
optional email relay
return OK
```

Fallimenti:

```text
No Wi-Fi -> offline fallback fortune
No OpenAI key -> localized setup/error state
STT timeout -> localized error + fallback
Brain JSON invalid -> localized error + fallback
TTS timeout -> show screen_answer anyway, skip voice or play local error cue
Playback unsupported -> show screen_answer, log format issue
```

## 10. Relay/server option, consigliata per voce finale

Se esiste già o se è facile aggiungerla, il relay dovrebbe poter fare anche voice generation, non solo email.

Vantaggi:

- API key non sta sul device in produzione.
- Conversione audio facile con ffmpeg/sox.
- Pause vere tra righe.
- Post-processing leggero.
- Realtime voice actor possibile senza mettere WebSocket complessi su ESP.

Endpoint possibile:

```text
POST /voice-oracle
```

Input:

```json
{
  "device_id": "...",
  "language": "it",
  "question_wav": "multipart upload or binary body",
  "settings": {
    "brain_model": "gpt-5.4-mini",
    "voice_provider": "audio_chat",
    "voice_model": "gpt-audio-1.5"
  }
}
```

Output:

```json
{
  "language": "it",
  "transcript": "...",
  "screen_answer": "...",
  "full_answer": "...",
  "audio_format": "wav",
  "audio_sample_rate": 16000,
  "audio_channels": 1,
  "audio_url": "optional if not inline"
}
```

Oppure risposta multipart:

```text
part 1: JSON metadata
part 2: answer.wav
```

Non bloccare la v1 su questo se il firmware direct è più vicino, ma progettare l'interfaccia in modo che il relay possa sostituire OpenAI direct.

## 11. Tool locale per testare la voce

Aggiungere un tool desktop, utile prima ancora del firmware:

```sh
python3 tools/test_bisc8_voice.py --provider audio_chat --lang it --out scratch/voice_tests
python3 tools/test_bisc8_voice.py --provider speech_tts --voice fable --out scratch/voice_tests
```

Il tool dovrebbe:

- Leggere `OPENAI_API_KEY` dall'ambiente.
- Generare 5 risposte campione in IT/EN/ES.
- Salvare WAV generati.
- Stampare latenza STT/Brain/TTS se si usa audio input reale.
- Non committare gli output.

Frasi test:

```text
Devo fidarmi di questa idea?
È il momento giusto per lanciare Bisc8?
Should I say yes to this strange little opportunity?
¿El oráculo de galleta aprueba mi plan?
```

Esempio script vocale:

```text
Mmm.
Le briciole hanno discusso.
Sì, ma aspetta il secondo segno.
```

## 12. Latency budget

Obiettivo realistico per v1:

```text
STT      2-6 s
Brain    1-4 s
Voice    2-8 s
Total    5-18 s
```

Per ridurre latenza:

- Brain max output breve.
- Prompt compatto.
- No streaming nella prima versione, salvo già pronto.
- Tenere `spoken_lines` brevi.
- Timeout e fallback locali.
- Evitare tre chiamate TTS separate in firmware.

## 13. Security

- Non loggare API key, bearer token, email relay token.
- Mascherare secrets nella web UI, mantenendo comportamento blank = keep existing value.
- Se produzione con secrets su device: flash encryption obbligatoria.
- Preferire relay/provisioning per build pubbliche.
- Non committare token in `sdkconfig.defaults`, sorgenti o public flasher.

## 14. Test richiesti

Mantenere i test esistenti verdi.

Aggiungere test unitari o host-side per:

- Parsing JSON risposta Brain.
- Troncamento UTF-8 sicuro di `screen_answer` a 100 caratteri.
- Fallback se JSON non valido.
- WAV header parser.
- Mono -> stereo duplication, se implementata.
- Errore su WAV non PCM o bit depth non supportata.
- No API key nei log.

Test manuali su device:

```text
1. Boot senza Wi-Fi -> fortune offline ancora funzionanti.
2. Boot con Wi-Fi ma senza OpenAI key -> errore/setup pulito.
3. Domanda IT -> risposta IT su e-paper, <= 100 char, audio riprodotto.
4. Domanda EN -> risposta EN, audio riprodotto.
5. Domanda ES -> risposta ES, audio riprodotto.
6. Timeout OpenAI -> fallback, niente crash.
7. Audio risposta lungo -> spool ok, RAM stabile.
8. PWR/deep sleep ancora funzionante dopo voice flow.
```

Build:

```sh
idf.py -C firmware/bisc8_fortune build
python -m pytest tests
```

## 15. Acceptance criteria

La prima PR è accettabile se:

- `VoiceOracleService::AskFromRecordedAudio()` non ritorna più `ESP_ERR_NOT_FINISHED` nel caso configurato.
- Una domanda registrata viene trascritta.
- Viene generato un JSON valido con `screen_answer` e `full_answer`.
- L'e-paper mostra `screen_answer` <= 100 caratteri.
- Viene generato audio WAV e salvato su spool.
- Bisc8 riproduce l'audio generato senza decoder compresso.
- Se una fase fallisce, il device non crasha e torna a uno stato utile.
- Offline fallback resta intatto.

La PR può lasciare `gpt-realtime-2` come esperimento/documentazione se non c'è tempo, ma deve lasciare l'architettura pronta a sostituire il voice provider.

## 16. Posizione netta

Per Bisc8, non puntare tutto su prompt TTS generici. La personalità deve venire da tre cose:

```text
1. Brain che scrive frasi già recitabili.
2. Voice provider con istruzioni da attore, non da assistente.
3. Audio pipeline che può aggiungere pause vere e convertire in formato firmware-friendly.
```

Implementazione consigliata:

```text
gpt-4o-mini-transcribe
  -> gpt-5.4-mini
  -> gpt-audio-1.5 se praticabile
  -> fallback gpt-4o-mini-tts
  -> WAV spool
  -> firmware playback
```

Realtime:

```text
gpt-realtime-2 solo come esperimento voice actor, meglio via relay/server.
```

Non far fare all'ESP32-C6 il lavoro di un fonico ubriaco in una cantina. Il device deve sembrare magico perché il backend e il firmware gli preparano un incantesimo già pulito.
