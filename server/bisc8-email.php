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
    echo json_encode(['ok' => true, 'service' => 'bisc8-email', 'ready' => ($cfg['token'] !== '' && $cfg['mail_to'] !== '')]);
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
             'q_file' => 'question.wav', 'a_file' => 'answer.wav',
             'days' => ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'],
             'months' => ['January','February','March','April','May','June','July','August','September','October','November','December']],
    'es' => ['subj_prefix' => "\u{1F36A} Bisc8: ", 'subj_fallback' => 'una lectura en las migas',
             'q_label' => 'Pregunta (transcrita)', 'a_label' => 'Leído en las migas',
             'empty' => '(vacío)', 'lang_label' => 'Idioma', 'date_label' => 'Fecha',
             'footer' => 'Desmigajado para ti por Bisc8', 'preheader' => 'Las migas han hablado.',
             'badge' => 'MIGAMANCIA', 'tagline' => 'adivinación entre las migas',
             'q_file' => 'pregunta.wav', 'a_file' => 'respuesta.wav',
             'days' => ['domingo','lunes','martes','miércoles','jueves','viernes','sábado'],
             'months' => ['enero','febrero','marzo','abril','mayo','junio','julio','agosto','septiembre','octubre','noviembre','diciembre']],
    'it' => ['subj_prefix' => "\u{1F36A} Bisc8: ", 'subj_fallback' => 'un responso tra le briciole',
             'q_label' => 'Domanda (trascritta)', 'a_label' => 'Letto nelle briciole',
             'empty' => '(vuoto)', 'lang_label' => 'Lingua', 'date_label' => 'Data',
             'footer' => 'Sbriciolato per te da Bisc8', 'preheader' => 'Le briciole hanno parlato.',
             'badge' => 'BRICIOMANZIA', 'tagline' => 'divinazione tra le briciole',
             'q_file' => 'domanda.wav', 'a_file' => 'risposta.wav',
             'days' => ['domenica','lunedì','martedì','mercoledì','giovedì','venerdì','sabato'],
             'months' => ['gennaio','febbraio','marzo','aprile','maggio','giugno','luglio','agosto','settembre','ottobre','novembre','dicembre']],
];
$S = $STRINGS[$lang] ?? $STRINGS['en'];

// Spelled-out local date+time, e.g. "Giovedì 5 giugno 2026, 18:42".
$now = time();
$dateFull = ucfirst($S['days'][(int)date('w', $now)]) . ' ' . (int)date('j', $now) . ' '
          . $S['months'][(int)date('n', $now) - 1] . ' ' . date('Y', $now) . ', ' . date('H:i', $now);

$subject = $S['subj_prefix'] . ($answerText !== '' ? mb_substr($answerText, 0, 60) : $S['subj_fallback']);
$text = $S['q_label'] . ":\n" . ($transcript !== '' ? $transcript : $S['empty']) . "\n\n"
      . $S['a_label'] . ":\n" . ($answerText !== '' ? $answerText : $S['empty']) . "\n"
      . "\n" . $S['date_label'] . ": " . $dateFull . "\n";
if ($lang !== '') {
    $text .= $S['lang_label'] . ": " . strtoupper($lang) . "\n";
}
$text .= "\n-- " . $S['footer'] . "\n";

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

$from = (string)$cfg['mail_from'] !== ''
    ? (string)$cfg['mail_from']
    : ('noreply@' . preg_replace('/[^a-z0-9.-]/i', '', $_SERVER['HTTP_HOST'] ?? 'localhost'));
$fromName = (string)($cfg['mail_from_name'] ?? 'Bisc8');

$headers = ['From: ' . ($fromName !== '' ? sprintf('"%s" <%s>', addcslashes($fromName, '"'), $from) : $from),
            'MIME-Version: 1.0'];

// --- HTML part: the "biscuit terminal" look from the web app, email-safe.
//     Pixelify Sans loads via Google Fonts (Apple Mail honours it); Gmail/Outlook
//     strip web fonts and fall back to monospace, which keeps the terminal feel.
//     User content is escaped; line breaks become <br>. ---
$esc = static fn(string $s): string => htmlspecialchars($s, ENT_QUOTES, 'UTF-8');
$qHtml = $transcript !== '' ? nl2br($esc($transcript)) : '<span style="font-style:italic;">' . $esc($S['empty']) . '</span>';
$aHtml = $answerText !== '' ? nl2br($esc($answerText)) : '<span style="font-style:italic;">' . $esc($S['empty']) . '</span>';
$langUpper = $lang !== '' ? strtoupper($esc($lang)) : '';
$chipsHtml = '';
// A 1-bit "waveform" (three ink bars) instead of a colour emoji, to stay monochrome.
$wave = '<span style="display:inline-block;vertical-align:middle;margin-right:6px;line-height:0;">'
    . '<span style="display:inline-block;width:2px;height:5px;background:#000;margin-right:1px;vertical-align:bottom;"></span>'
    . '<span style="display:inline-block;width:2px;height:9px;background:#000;margin-right:1px;vertical-align:bottom;"></span>'
    . '<span style="display:inline-block;width:2px;height:6px;background:#000;vertical-align:bottom;"></span>'
    . '</span>';
foreach ($attachments as $att) {
    $chipsHtml .= '<span style="display:inline-block;font-family:ui-monospace,\'Courier New\',monospace;'
        . 'font-size:12px;border:2px solid #000;box-shadow:2px 2px 0 #000;padding:4px 9px;margin:0 8px 8px 0;'
        . 'color:#000;background:#fff;">' . $wave . $esc($att['filename']) . '</span>';
}
$qLabel = $esc($S['q_label']);
$aLabel = $esc($S['a_label']);
$badge = $esc($S['badge']);
$tagline = $esc($S['tagline']);
$footer = $esc($S['footer']);
$langLabel = $esc($S['lang_label']);
$preheader = $esc($S['preheader']);
$dateFullEsc = $esc($dateFull);
$PXF = "'Pixelify Sans',ui-monospace,'Courier New',monospace";  // heading stack
$MNF = "ui-monospace,'Courier New',monospace";                   // mono/body stack
$footerLeft = $langUpper !== '' ? "{$langLabel}: {$langUpper}" : '&nbsp;';
$chipsRow = $chipsHtml !== '' ? "<tr><td style=\"padding:0 20px 16px;\">{$chipsHtml}</td></tr>" : '';

$html = <<<HTML
<!doctype html>
<html lang="{$lang}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Pixelify+Sans:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>body{margin:0;padding:0;background:#cfc9bd;-webkit-text-size-adjust:100%;}</style>
</head>
<body style="margin:0;padding:0;background:#cfc9bd;">
<div style="display:none;max-height:0;overflow:hidden;mso-hide:all;opacity:0;color:#cfc9bd;font-size:1px;line-height:1px;">{$preheader}</div>
<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#cfc9bd;">
<tr><td align="center" style="padding:26px 14px;">
<table role="presentation" width="520" cellpadding="0" cellspacing="0" style="width:100%;max-width:520px;background:#fff;border:2px solid #000;box-shadow:6px 6px 0 rgba(0,0,0,.35);">

<tr><td style="background:#000;padding:14px 18px;">
  <table role="presentation" width="100%" cellpadding="0" cellspacing="0"><tr>
    <td style="font-family:{$PXF};color:#fff;font-size:22px;font-weight:700;letter-spacing:2px;"><img src="cid:bisc8logo" width="36" height="36" alt="Bisc8" style="vertical-align:middle;margin-right:10px;background:#fff;">Bisc8<span style="font-family:{$MNF};font-size:11px;letter-spacing:2px;">  &middot; {$badge}</span></td>
    <td align="right" style="font-family:{$MNF};color:#fff;font-size:10px;letter-spacing:1px;text-transform:uppercase;">{$tagline}</td>
  </tr></table>
</td></tr>

<tr><td style="padding:22px 20px 6px;">
  <div style="font-family:{$MNF};font-size:10px;letter-spacing:2px;text-transform:uppercase;color:#000;margin:0 0 6px;">{$qLabel}</div>
  <table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="border:2px solid #000;margin:0 0 20px;"><tr>
    <td style="font-family:{$MNF};font-size:15px;line-height:1.55;color:#000;padding:12px 14px;">{$qHtml}</td>
  </tr></table>

  <div style="font-family:{$MNF};font-size:10px;letter-spacing:2px;text-transform:uppercase;color:#000;margin:0 0 6px;">{$aLabel}</div>
  <table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#000;border:2px solid #000;box-shadow:4px 4px 0 rgba(0,0,0,.3);margin:0 0 18px;"><tr>
    <td style="font-family:{$PXF};font-size:19px;line-height:1.6;color:#fff;padding:18px;">{$aHtml}</td>
  </tr></table>
</td></tr>

{$chipsRow}

<tr><td style="padding:4px 20px 2px;text-align:center;">
  <span style="font-family:{$MNF};font-size:11px;letter-spacing:1px;color:#000;">{$dateFullEsc}</span>
</td></tr>

<tr><td style="padding:10px 20px 18px;border-top:2px dotted #000;">
  <table role="presentation" width="100%" cellpadding="0" cellspacing="0"><tr>
    <td style="font-family:{$MNF};font-size:10px;letter-spacing:1px;text-transform:uppercase;color:#000;">{$footerLeft}</td>
    <td align="right" style="font-family:{$MNF};font-size:10px;letter-spacing:1px;text-transform:uppercase;color:#000;">{$footer}</td>
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
// Base64 of assets/logo/logo_min.png, embedded so the relay stays a single
// self-contained file (served inline as cid:bisc8logo). Injected by build tooling.
$BISC8_LOGO_B64 = 'iVBORw0KGgoAAAANSUhEUgAAAGQAAABkCAIAAAD/gAIDAAAACXBIWXMAAAsTAAALEwEAmpwYAAAGI2lUWHRYTUw6Y29tLmFkb2JlLnhtcAAAAAAAPD94cGFja2V0IGJlZ2luPSLvu78iIGlkPSJXNU0wTXBDZWhpSHpyZVN6TlRjemtjOWQiPz4gPHg6eG1wbWV0YSB4bWxuczp4PSJhZG9iZTpuczptZXRhLyIgeDp4bXB0az0iQWRvYmUgWE1QIENvcmUgMTAuMC1jMDAwIDc5LmQyMGU0NjYzMCwgMjAyNS8xMi8wOS0wMjoxMToyMyAgICAgICAgIj4gPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4gPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIgeG1sbnM6eG1wPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvIiB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iIHhtbG5zOnBob3Rvc2hvcD0iaHR0cDovL25zLmFkb2JlLmNvbS9waG90b3Nob3AvMS4wLyIgeG1sbnM6eG1wTU09Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9tbS8iIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiIHhtcDpDcmVhdG9yVG9vbD0iQWRvYmUgUGhvdG9zaG9wIDI3LjggKDIwMjYwNTI1Lm0uMzU1MyAxNjcxODgwKSAgKE1hY2ludG9zaCkiIHhtcDpDcmVhdGVEYXRlPSIyMDI2LTA2LTA0VDEyOjU0OjEzKzAyOjAwIiB4bXA6TW9kaWZ5RGF0ZT0iMjAyNi0wNi0wNFQxMjo1ODowNSswMjowMCIgeG1wOk1ldGFkYXRhRGF0ZT0iMjAyNi0wNi0wNFQxMjo1ODowNSswMjowMCIgZGM6Zm9ybWF0PSJpbWFnZS9wbmciIHBob3Rvc2hvcDpDb2xvck1vZGU9IjMiIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6MWNhNjYxOTYtNTY4MC00NmU1LWJjNTAtMTdiNmQzZGIyMjc4IiB4bXBNTTpEb2N1bWVudElEPSJhZG9iZTpkb2NpZDpwaG90b3Nob3A6NTBhY2Q2MzYtNzQxNC1jNTQ2LTkyMDgtYTRjMDI0MmNlY2Q3IiB4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ9InhtcC5kaWQ6Y2FjMTUxYzctZWU4Ny00ZDIwLThkYTMtOWI5YzIzZjMzYjhlIj4gPHhtcE1NOkhpc3Rvcnk+IDxyZGY6U2VxPiA8cmRmOmxpIHN0RXZ0OmFjdGlvbj0iY3JlYXRlZCIgc3RFdnQ6aW5zdGFuY2VJRD0ieG1wLmlpZDpjYWMxNTFjNy1lZTg3LTRkMjAtOGRhMy05YjljMjNmMzNiOGUiIHN0RXZ0OndoZW49IjIwMjYtMDYtMDRUMTI6NTQ6MTMrMDI6MDAiIHN0RXZ0OnNvZnR3YXJlQWdlbnQ9IkFkb2JlIFBob3Rvc2hvcCAyNy44ICgyMDI2MDUyNS5tLjM1NTMgMTY3MTg4MCkgIChNYWNpbnRvc2gpIi8+IDxyZGY6bGkgc3RFdnQ6YWN0aW9uPSJzYXZlZCIgc3RFdnQ6aW5zdGFuY2VJRD0ieG1wLmlpZDoxY2E2NjE5Ni01NjgwLTQ2ZTUtYmM1MC0xN2I2ZDNkYjIyNzgiIHN0RXZ0OndoZW49IjIwMjYtMDYtMDRUMTI6NTg6MDUrMDI6MDAiIHN0RXZ0OnNvZnR3YXJlQWdlbnQ9IkFkb2JlIFBob3Rvc2hvcCAyNy44ICgyMDI2MDUyNS5tLjM1NTMgMTY3MTg4MCkgIChNYWNpbnRvc2gpIiBzdEV2dDpjaGFuZ2VkPSIvIi8+IDwvcmRmOlNlcT4gPC94bXBNTTpIaXN0b3J5PiA8L3JkZjpEZXNjcmlwdGlvbj4gPC9yZGY6UkRGPiA8L3g6eG1wbWV0YT4gPD94cGFja2V0IGVuZD0iciI/Ph9kk2MAACjRSURBVHiclX0HfFVF1vi8R5KXIIZEpCgREjpKgBRqqFIVBJVQDDaUYlmVpSji6t+1i64F3EUUREFWxZ+IiIAoQUGKoShNRUhCT0ghJCE97873mzkzZ87c+/D/fUNI3rt37syZM2dOn7k+x3EY58znY1g4F7/hivgovzLxR1xSNbn87hOPiov4lZM6THwVn9Wj6i5U4LJX0q3uWT6iQSIN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';

$altB = 'alt_' . bin2hex(random_bytes(8));
$alt  = "--{$altB}\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Transfer-Encoding: 8bit\r\n\r\n" . $text . "\r\n";
$alt .= "--{$altB}\r\nContent-Type: text/html; charset=utf-8\r\nContent-Transfer-Encoding: base64\r\n\r\n" . chunk_split(base64_encode($html)) . "\r\n";
$alt .= "--{$altB}--\r\n";

// Wrap the alternative + the inline mascot (referenced as cid:bisc8logo in the
// HTML header) in a multipart/related. CID images render where data: URIs are
// blocked (notably Gmail); if a client strips the image, the <img alt> shows.
$relB = 'rel_' . bin2hex(random_bytes(8));
$related  = "--{$relB}\r\nContent-Type: multipart/alternative; boundary=\"{$altB}\"\r\n\r\n" . $alt . "\r\n";
$related .= "--{$relB}\r\nContent-Type: image/png; name=\"bisc8.png\"\r\n"
          . "Content-Transfer-Encoding: base64\r\nContent-ID: <bisc8logo>\r\n"
          . "Content-Disposition: inline; filename=\"bisc8.png\"\r\n\r\n"
          . chunk_split($BISC8_LOGO_B64) . "\r\n";
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
if ($ok) {
    echo json_encode(['ok' => true, 'attached' => count($attachments)]);
} else {
    http_response_code(502);
    echo json_encode(['ok' => false, 'error' => 'mail() failed']);
}
