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
    . '<span style="display:inline-block;width:2px;height:5px;background:#23211c;margin-right:1px;vertical-align:bottom;"></span>'
    . '<span style="display:inline-block;width:2px;height:9px;background:#23211c;margin-right:1px;vertical-align:bottom;"></span>'
    . '<span style="display:inline-block;width:2px;height:6px;background:#23211c;vertical-align:bottom;"></span>'
    . '</span>';
foreach ($attachments as $att) {
    $chipsHtml .= '<span style="display:inline-block;font-family:\'Pixolde\',ui-monospace,\'Courier New\',monospace;'
        . 'font-size:16px;border:2px solid #23211c;box-shadow:inset 0 2px 0 rgba(255,255,255,.55),3px 3px 0 #23211c;padding:10px 16px;margin:0 10px 10px 0;'
        . 'color:#23211c;background:#f4e4a0;">' . $wave . $esc($att['filename']) . '</span>';
}
$qLabel = $esc($S['q_label']);
$aLabel = $esc($S['a_label']);
$badge = $esc($S['badge']);
$tagline = $esc($S['tagline']);
$footer = $esc($S['footer']);
$langLabel = $esc($S['lang_label']);
$preheader = $esc($S['preheader']);
$dateFullEsc = $esc($dateFull);
$PXF  = "'ChiKareGo2',ui-monospace,'Courier New',monospace";  // titles / labels / wordmark (site pixel font)
$MNF  = "'Pixolde',ui-monospace,'Courier New',monospace";      // body / question / footer (site pixel font)
$LONG = "Georgia,'Times New Roman',serif";                     // the reading (serif; free Garamond stand-in)
$footerLeft = $langUpper !== '' ? "{$langLabel}: {$langUpper}" : '&nbsp;';
$chipsRow = $chipsHtml !== '' ? "<tr><td style=\"padding:0 20px 16px;\">{$chipsHtml}</td></tr>" : '';

$html = <<<HTML
<!doctype html>
<html lang="{$lang}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
@font-face{font-family:'ChiKareGo2';src:url(data:font/woff2;base64,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) format('woff2');font-weight:400;font-display:swap;}
@font-face{font-family:'Pixolde';src:url(data:font/woff2;base64,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) format('woff2');font-weight:400 700;font-display:swap;}
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
  <div style="font-family:{$PXF};font-size:16px;letter-spacing:1px;color:#23211c;margin:0 0 8px;">{$qLabel}</div>
  <table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#fbf6ef;border:2px solid #23211c;box-shadow:inset 0 2px 0 rgba(255,255,255,.6),4px 4px 0 rgba(35,33,28,.22);margin:0 0 22px;"><tr>
    <td style="font-family:{$MNF};font-size:18px;line-height:1.5;color:#23211c;padding:16px 18px;">{$qHtml}</td>
  </tr></table>

  <div style="font-family:{$PXF};font-size:16px;letter-spacing:1px;color:#23211c;margin:0 0 8px;">{$aLabel}</div>
  <table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#fbf6ef;border:2px solid #23211c;box-shadow:inset 0 2px 0 rgba(255,255,255,.6),5px 5px 0 rgba(35,33,28,.26);margin:0 0 18px;"><tr>
    <td style="font-family:{$LONG};font-size:18px;line-height:1.55;color:#23211c;padding:16px 18px 18px;">{$aHtml}</td>
  </tr></table>
</td></tr>

{$chipsRow}

<tr><td style="padding:4px 20px 2px;text-align:center;">
  <span style="font-family:{$MNF};font-size:16px;letter-spacing:1px;color:#23211c;">{$dateFullEsc}</span>
</td></tr>

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
// No inline images: the email is self-contained, fonts embedded via @font-face.
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
if ($ok) {
    echo json_encode(['ok' => true, 'attached' => count($attachments)]);
} else {
    http_response_code(502);
    echo json_encode(['ok' => false, 'error' => 'mail() failed']);
}
