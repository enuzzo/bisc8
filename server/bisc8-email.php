<?php
declare(strict_types=1);

// Bisc8 e-paper oracle -> email relay (standalone).
//
// The Bisc8 device POSTs here (multipart/form-data) after a voice query to have
// its transcribed question + the oracle's answer emailed, with the original
// recording (question.wav) attached. No framework, no dependency on any other
// app, and NO secrets in this file: configuration lives in a sibling
// `bisc8-email.config.php` (copy from the .example, keep it out of git) or in
// environment variables. Deliverability uses the host's own mail() (or whatever
// your host wires mail() to), so no email credentials ever live on the device.
//
// Deploy: upload this file + your bisc8-email.config.php to any PHP host folder,
// e.g.  https://your-host/.../api/bisc8-email.php
// Then in the Bisc8 captive portal (Email section):
//   Relay URL   = the full URL to this file
//   Relay token = the same `token` value from your config

header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store');
header('X-Content-Type-Options: nosniff');

const BISC8_EMAIL_RELAY_VERSION = '2026-06-12-pre-mime-ack';
const BISC8_EMAIL_LOG = __DIR__ . '/bisc8-email.log.php';

ignore_user_abort(true);
if (function_exists('set_time_limit')) {
    @set_time_limit(180);
}

function relay_log(array $fields): void
{
    $row = array_merge(['ts' => date('c'), 'version' => BISC8_EMAIL_RELAY_VERSION], $fields);
    $line = json_encode($row, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE);
    if ($line === false) {
        return;
    }
    if (!is_file(BISC8_EMAIL_LOG)) {
        @file_put_contents(BISC8_EMAIL_LOG, "<?php exit; ?>\n", FILE_APPEND | LOCK_EX);
    }
    @file_put_contents(BISC8_EMAIL_LOG, $line . PHP_EOL, FILE_APPEND | LOCK_EX);
}

function respond_json_and_continue(array $payload, int $status = 202): void
{
    $json = json_encode($payload, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE);
    if ($json === false) {
        $json = '{"ok":true,"accepted":true}';
    }

    http_response_code($status);
    header('Content-Type: application/json; charset=utf-8');
    header('Cache-Control: no-store');
    header('X-Content-Type-Options: nosniff');
    header('Connection: close');
    header('Content-Length: ' . strlen($json));
    echo $json;

    if (function_exists('fastcgi_finish_request')) {
        fastcgi_finish_request();
        return;
    }
    while (ob_get_level() > 0) {
        @ob_end_flush();
    }
    flush();
}

function relay_subject_preview(string $value, int $maxChars): string
{
    if (function_exists('mb_substr')) {
        return mb_substr($value, 0, $maxChars, 'UTF-8');
    }
    return substr($value, 0, $maxChars);
}

// --- Config: file first, then env-var overrides. Nothing secret is committed. ---
$cfg = ['token' => '', 'mail_to' => '', 'mail_from' => '', 'mail_from_name' => 'Bisc8', 'max_attach_mb' => 9, 'timezone' => 'Europe/Rome'];
$cfgFile = __DIR__ . '/bisc8-email.config.php';
if (is_file($cfgFile)) {
    $loaded = require $cfgFile;
    if (is_array($loaded)) {
        $cfg = array_merge($cfg, $loaded);
    }
}
foreach (['token' => 'BISC8_RELAY_TOKEN', 'mail_to' => 'BISC8_MAIL_TO', 'mail_from' => 'BISC8_MAIL_FROM'] as $k => $envName) {
    $v = getenv($envName);
    if ($v !== false && $v !== '') {
        $cfg[$k] = $v;
    }
}

// Local time for the email's timestamp (override via config 'timezone').
date_default_timezone_set((string)($cfg['timezone'] ?? '') !== '' ? (string)$cfg['timezone'] : 'Europe/Rome');

$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';

// Health check: open the URL in a browser to confirm it deployed and is ready.
if ($method === 'GET') {
    echo json_encode(['ok' => true, 'service' => 'bisc8-email', 'version' => BISC8_EMAIL_RELAY_VERSION, 'ready' => ($cfg['token'] !== '' && $cfg['mail_to'] !== '')]);
    exit;
}
if ($method !== 'POST') {
    http_response_code(405);
    echo json_encode(['ok' => false, 'error' => 'Method not allowed']);
    exit;
}

// --- Auth: shared bearer token (header, or `token` field if the host strips it). ---
$auth = $_SERVER['HTTP_AUTHORIZATION'] ?? ($_SERVER['REDIRECT_HTTP_AUTHORIZATION'] ?? '');
$given = (stripos($auth, 'Bearer ') === 0) ? trim(substr($auth, 7)) : (string)($_POST['token'] ?? '');
if ($cfg['token'] === '' || $given === '' || !hash_equals((string)$cfg['token'], $given)) {
    http_response_code(401);
    echo json_encode(['ok' => false, 'error' => 'Unauthorized']);
    exit;
}

// --- Recipient: prefer the config's fixed address so a leaked token can't spam
//     arbitrary recipients; fall back to the device-provided value. ---
$to = (string)$cfg['mail_to'] !== '' ? (string)$cfg['mail_to'] : trim((string)($_POST['to'] ?? ''));
if (!filter_var($to, FILTER_VALIDATE_EMAIL)) {
    http_response_code(400);
    echo json_encode(['ok' => false, 'error' => 'Missing or invalid recipient']);
    exit;
}

$transcript = trim((string)($_POST['transcript'] ?? ''));
$answerText = trim((string)($_POST['answer'] ?? ''));
$lang = strtolower(trim((string)($_POST['lang'] ?? '')));
// Engines used per phase (for the "made with" line). Device-provided, optional.
$sttModel   = trim((string)($_POST['stt_model'] ?? ''));
$brainModel = trim((string)($_POST['brain_model'] ?? ''));
$ttsModel   = trim((string)($_POST['tts_model'] ?? ''));
$voiceName  = trim((string)($_POST['voice'] ?? ''));
$engineBits = [];
if ($sttModel   !== '') $engineBits[] = 'STT '   . $sttModel;
if ($brainModel !== '') $engineBits[] = 'brain ' . $brainModel;
if ($ttsModel   !== '') $engineBits[] = 'TTS '   . $ttsModel;
if ($voiceName  !== '') $engineBits[] = 'voice ' . $voiceName;

// --- Localized chrome: subject, body labels and attachment filenames follow the
//     language the device detected for THIS query (falls back to English). The
//     audio itself is already in that language (the oracle answers in kind). ---
// Pastry-divination voice: the oracle reads your fate in the crumbs ("-mancy"
// coined like cartomancy). days/months feed the spelled-out local date.
$STRINGS = [
    'en' => ['subj_prefix' => "\u{1F36A} Bisc8: ", 'subj_fallback' => 'a reading in the crumbs',
             'q_label' => 'Question (transcribed)', 'a_label' => 'Read in the crumbs',
             'empty' => '(empty)', 'lang_label' => 'Language', 'date_label' => 'Date',
             'footer' => 'Crumbled for you by Bisc8', 'preheader' => 'The crumbs have spoken.',
             'badge' => 'CRUMBOMANCY', 'tagline' => 'fortunes read in the crumbs',
             'q_file' => 'question.wav', 'a_file' => 'answer.wav', 'attach_label' => 'Attached',
             'days' => ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'],
             'months' => ['January','February','March','April','May','June','July','August','September','October','November','December']],
    'es' => ['subj_prefix' => "\u{1F36A} Bisc8: ", 'subj_fallback' => 'una lectura en las migas',
             'q_label' => 'Pregunta (transcrita)', 'a_label' => 'Leído en las migas',
             'empty' => '(vacío)', 'lang_label' => 'Idioma', 'date_label' => 'Fecha',
             'footer' => 'Desmigajado para ti por Bisc8', 'preheader' => 'Las migas han hablado.',
             'badge' => 'MIGAMANCIA', 'tagline' => 'adivinación entre las migas',
             'q_file' => 'pregunta.wav', 'a_file' => 'respuesta.wav', 'attach_label' => 'Adjuntos',
             'days' => ['domingo','lunes','martes','miércoles','jueves','viernes','sábado'],
             'months' => ['enero','febrero','marzo','abril','mayo','junio','julio','agosto','septiembre','octubre','noviembre','diciembre']],
    'it' => ['subj_prefix' => "\u{1F36A} Bisc8: ", 'subj_fallback' => 'un responso tra le briciole',
             'q_label' => 'Domanda (trascritta)', 'a_label' => 'Letto nelle briciole',
             'empty' => '(vuoto)', 'lang_label' => 'Lingua', 'date_label' => 'Data',
             'footer' => 'Sbriciolato per te da Bisc8', 'preheader' => 'Le briciole hanno parlato.',
             'badge' => 'BRICIOMANZIA', 'tagline' => 'divinazione tra le briciole',
             'q_file' => 'domanda.wav', 'a_file' => 'risposta.wav', 'attach_label' => 'Allegati',
             'days' => ['domenica','lunedì','martedì','mercoledì','giovedì','venerdì','sabato'],
             'months' => ['gennaio','febbraio','marzo','aprile','maggio','giugno','luglio','agosto','settembre','ottobre','novembre','dicembre']],
];
$S = $STRINGS[$lang] ?? $STRINGS['en'];

// Spelled-out local date+time, e.g. "Giovedì 5 giugno 2026, 18:42".
$now = time();
$dateFull = ucfirst($S['days'][(int)date('w', $now)]) . ' ' . (int)date('j', $now) . ' '
          . $S['months'][(int)date('n', $now) - 1] . ' ' . date('Y', $now) . ', ' . date('H:i', $now);

// --- Optional WAV attachments: the question recording (field "audio") and the
//     generated answer audio (field "answer"). Either may be absent. ---
$maxBytes = ((int)$cfg['max_attach_mb']) * 1024 * 1024;
$readUpload = static function (string $field) use ($maxBytes): ?string {
    if (empty($_FILES[$field]) || (int)($_FILES[$field]['error'] ?? UPLOAD_ERR_NO_FILE) !== UPLOAD_ERR_OK) {
        return null;
    }
    $tmp = (string)$_FILES[$field]['tmp_name'];
    $size = (int)($_FILES[$field]['size'] ?? 0);
    if ($size <= 0 || $size > $maxBytes || !is_uploaded_file($tmp)) {
        return null;
    }
    $data = file_get_contents($tmp);
    return ($data !== false && $data !== '') ? $data : null;
};

$attachments = [];
$qWav = $readUpload('audio');
if ($qWav !== null) {
    $attachments[] = ['filename' => $S['q_file'], 'data' => $qWav];
}
$aWav = $readUpload('answer_audio');  // the answer TTS; "answer" (text) lives in $answerText
if ($aWav !== null) {
    $attachments[] = ['filename' => $S['a_file'], 'data' => $aWav];
}

$requestId = bin2hex(random_bytes(6));
$attachmentInfo = array_map(static fn(array $att): array => [
    'filename' => (string)$att['filename'],
    'bytes' => strlen((string)$att['data']),
], $attachments);

respond_json_and_continue(['ok' => true, 'accepted' => true, 'attached' => count($attachments), 'request_id' => $requestId, 'version' => BISC8_EMAIL_RELAY_VERSION]);

relay_log([
    'id' => $requestId,
    'event' => 'accepted',
    'attached' => count($attachments),
    'attachments' => $attachmentInfo,
    'content_length' => (int)($_SERVER['CONTENT_LENGTH'] ?? 0),
    'lang' => $lang,
    'stt_model' => $sttModel,
    'brain_model' => $brainModel,
    'tts_model' => $ttsModel,
    'voice' => $voiceName,
]);

// The WAVs ride along as real MIME attachments; an email client has no stable URL
// to "click", so we simply name them (no fake buttons) in both the text and HTML.
$subject = $S['subj_prefix'] . ($answerText !== '' ? relay_subject_preview($answerText, 60) : $S['subj_fallback']);
$text = $S['q_label'] . ":\n" . ($transcript !== '' ? $transcript : $S['empty']) . "\n\n"
      . $S['a_label'] . ":\n" . ($answerText !== '' ? $answerText : $S['empty']) . "\n"
      . "\n" . $S['date_label'] . ": " . $dateFull . "\n";
if ($lang !== '') {
    $text .= $S['lang_label'] . ": " . strtoupper($lang) . "\n";
}
if ($engineBits) {
    $text .= "engines: " . implode(' · ', $engineBits) . "\n";
}
$attachNames = array_map(static fn(array $a): string => (string)$a['filename'], $attachments);
$attachNote  = $attachNames ? ($S['attach_label'] . ': ' . implode(', ', $attachNames)) : '';
if ($attachNote !== '') {
    $text .= $attachNote . "\n";
}
$text .= "\n-- " . $S['footer'] . "\n";

$from = (string)$cfg['mail_from'] !== ''
    ? (string)$cfg['mail_from']
    : ('noreply@' . preg_replace('/[^a-z0-9.-]/i', '', $_SERVER['HTTP_HOST'] ?? 'localhost'));
$fromName = (string)($cfg['mail_from_name'] ?? 'Bisc8');

$headers = ['From: ' . ($fromName !== '' ? sprintf('"%s" <%s>', addcslashes($fromName, '"'), $from) : $from),
            'MIME-Version: 1.0'];

// --- HTML part: the "biscuit terminal" look from the web app, email-safe.
//     User content is escaped; line breaks become <br>. The email intentionally
//     uses common fallback fonts: audio attachments already dominate the MIME,
//     so embedding whole web fonts makes PHP mail() much more fragile.
$esc = static fn(string $s): string => htmlspecialchars($s, ENT_QUOTES, 'UTF-8');
$qHtml = $transcript !== '' ? nl2br($esc($transcript)) : '<span style="font-style:italic;">' . $esc($S['empty']) . '</span>';
$aHtml = $answerText !== '' ? nl2br($esc($answerText)) : '<span style="font-style:italic;">' . $esc($S['empty']) . '</span>';
$langUpper = $lang !== '' ? strtoupper($esc($lang)) : '';
$qLabel = $esc($S['q_label']);
$aLabel = $esc($S['a_label']);
$badge = $esc($S['badge']);
$tagline = $esc($S['tagline']);
$footer = $esc($S['footer']);
$langLabel = $esc($S['lang_label']);
$preheader = $esc($S['preheader']);
$dateFullEsc = $esc($dateFull);
$PXF  = "ui-monospace,'Courier New',monospace";  // titles / labels / wordmark
$MNF  = "ui-monospace,'Courier New',monospace";  // body / question / footer
$LONG = "Georgia,'Times New Roman',serif";                     // the reading (serif; free Garamond stand-in)
// "Made with" engines line (STT / brain / TTS / voice), HTML row.
$enginesEsc = $esc(implode('  ·  ', $engineBits));
$enginesRow = $engineBits
    ? "<tr><td style=\"padding:0 20px 10px;text-align:center;\"><span style=\"font-family:{$MNF};font-size:16px;letter-spacing:.4px;color:#8a7d70;\">{$enginesEsc}</span></td></tr>"
    : '';
$footerLeft = $langUpper !== '' ? "{$langLabel}: {$langUpper}" : '&nbsp;';
$attachRow = $attachNote !== ''
    ? "<tr><td style=\"padding:2px 20px 16px;text-align:center;\"><span style=\"font-family:{$MNF};font-size:16px;letter-spacing:.4px;color:#23211c;\">" . $esc($attachNote) . "</span></td></tr>"
    : '';

$html = <<<HTML
<!doctype html>
<html lang="{$lang}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{margin:0;padding:0;background:#f0d9d2;-webkit-text-size-adjust:100%;}
</style>
</head>
<body style="margin:0;padding:0;background:#f0d9d2;">
<div style="display:none;max-height:0;overflow:hidden;mso-hide:all;opacity:0;color:#f0d9d2;font-size:1px;line-height:1px;">{$preheader}</div>
<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#f0d9d2;">
<tr><td align="center" style="padding:26px 14px;">
<table role="presentation" width="520" cellpadding="0" cellspacing="0" style="width:100%;max-width:520px;background:#fbf6ef;border:2px solid #23211c;box-shadow:6px 6px 0 rgba(0,0,0,.35);">

<tr><td style="background:#fbf6ef;background-image:repeating-linear-gradient(#23211c 0 1px,#fbf6ef 1px 3px);border-bottom:2px solid #23211c;padding:12px 16px;text-align:center;">
  <span style="display:inline-block;font-family:{$PXF};color:#23211c;font-size:19px;letter-spacing:1px;background:#fbf6ef;border:2px solid #23211c;box-shadow:2px 2px 0 rgba(35,33,28,.25);padding:3px 14px;">Bisc8 <span style="font-family:{$MNF};font-size:16px;letter-spacing:1px;">&middot; {$badge}</span></span>
</td></tr>

<tr><td style="height:4px;background:#f4e4a0;font-size:0;line-height:0;">&nbsp;</td></tr>

<tr><td style="padding:22px 20px 6px;">
  <div style="font-family:{$PXF};font-size:21px;letter-spacing:1px;color:#23211c;margin:0 0 8px;">{$qLabel}</div>
  <table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#fbf6ef;border:2px solid #23211c;box-shadow:inset 0 2px 0 rgba(255,255,255,.6),4px 4px 0 rgba(35,33,28,.22);margin:0 0 22px;"><tr>
    <td style="font-family:{$LONG};font-size:18px;line-height:1.55;color:#23211c;padding:16px 18px;">{$qHtml}</td>
  </tr></table>

  <div style="font-family:{$PXF};font-size:21px;letter-spacing:1px;color:#23211c;margin:0 0 8px;">{$aLabel}</div>
  <table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#fbf6ef;border:2px solid #23211c;box-shadow:inset 0 2px 0 rgba(255,255,255,.6),5px 5px 0 rgba(35,33,28,.26);margin:0 0 18px;"><tr>
    <td style="font-family:{$LONG};font-size:18px;line-height:1.55;color:#23211c;padding:16px 18px 18px;">{$aHtml}</td>
  </tr></table>
</td></tr>

{$attachRow}

<tr><td style="padding:4px 20px 2px;text-align:center;">
  <span style="font-family:{$MNF};font-size:16px;letter-spacing:1px;color:#23211c;">{$dateFullEsc}</span>
</td></tr>

{$enginesRow}

<tr><td style="padding:10px 20px 18px;border-top:2px dotted #23211c;">
  <table role="presentation" width="100%" cellpadding="0" cellspacing="0"><tr>
    <td style="font-family:{$MNF};font-size:16px;letter-spacing:.5px;text-transform:uppercase;color:#23211c;">{$footerLeft}</td>
    <td align="right" style="font-family:{$MNF};font-size:16px;letter-spacing:.5px;text-transform:uppercase;color:#23211c;">{$footer}</td>
  </tr></table>
</td></tr>

</table>
</td></tr>
</table>
</body>
</html>
HTML;

// --- Assemble MIME: multipart/alternative (plain + HTML); wrapped in
//     multipart/mixed when there are WAV attachments. ---

$altB = 'alt_' . bin2hex(random_bytes(8));
$alt  = "--{$altB}\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Transfer-Encoding: 8bit\r\n\r\n" . $text . "\r\n";
$alt .= "--{$altB}\r\nContent-Type: text/html; charset=utf-8\r\nContent-Transfer-Encoding: base64\r\n\r\n" . chunk_split(base64_encode($html)) . "\r\n";
$alt .= "--{$altB}--\r\n";

// Wrap the alternative in a multipart/related (kept for structural stability).
// No inline images and no embedded fonts: keep the email self-contained but light.
$relB = 'rel_' . bin2hex(random_bytes(8));
$related  = "--{$relB}\r\nContent-Type: multipart/alternative; boundary=\"{$altB}\"\r\n\r\n" . $alt . "\r\n";
$related .= "--{$relB}--\r\n";

if (!empty($attachments)) {
    $mixB = 'mix_' . bin2hex(random_bytes(8));
    $headers[] = 'Content-Type: multipart/mixed; boundary="' . $mixB . '"';
    $body = "--{$mixB}\r\nContent-Type: multipart/related; boundary=\"{$relB}\"\r\n\r\n" . $related . "\r\n";
    foreach ($attachments as $att) {
        $fn = $att['filename'];
        $body .= "--{$mixB}\r\nContent-Type: audio/wav; name=\"{$fn}\"\r\n"
               . "Content-Transfer-Encoding: base64\r\nContent-Disposition: attachment; filename=\"{$fn}\"\r\n\r\n"
               . chunk_split(base64_encode($att['data'])) . "\r\n";
    }
    $body .= "--{$mixB}--\r\n";
} else {
    $headers[] = 'Content-Type: multipart/related; boundary="' . $relB . '"';
    $body = $related;
}

$ok = @mail($to, $subject, $body, implode("\r\n", $headers));
$lastError = error_get_last();
relay_log([
    'id' => $requestId,
    'event' => 'mail_result',
    'ok' => (bool)$ok,
    'attached' => count($attachments),
    'error' => $ok ? null : (string)($lastError['message'] ?? 'mail() failed'),
]);
