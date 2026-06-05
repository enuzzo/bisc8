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
$cfg = ['token' => '', 'mail_to' => '', 'mail_from' => '', 'mail_from_name' => 'Bisc8', 'max_attach_mb' => 9];
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
$answer = trim((string)($_POST['answer'] ?? ''));
$lang = trim((string)($_POST['lang'] ?? ''));

$subject = 'Bisc8 oracolo: ' . ($answer !== '' ? mb_substr($answer, 0, 60) : 'responso');
$text = "Domanda (trascritta):\n" . ($transcript !== '' ? $transcript : '(vuota)') . "\n\n"
      . "Responso:\n" . ($answer !== '' ? $answer : '(vuoto)') . "\n";
if ($lang !== '') {
    $text .= "\nLingua: " . $lang . "\n";
}
$text .= "\n-- Inviato dal tuo Bisc8\n";

// --- Optional WAV attachment (the original question recording). ---
$wav = null;
if (!empty($_FILES['audio']) && (int)($_FILES['audio']['error'] ?? UPLOAD_ERR_NO_FILE) === UPLOAD_ERR_OK) {
    $tmp = (string)$_FILES['audio']['tmp_name'];
    $size = (int)($_FILES['audio']['size'] ?? 0);
    $max = ((int)$cfg['max_attach_mb']) * 1024 * 1024;
    if ($size > 0 && $size <= $max && is_uploaded_file($tmp)) {
        $data = file_get_contents($tmp);
        if ($data !== false && $data !== '') {
            $wav = $data;
        }
    }
}

$from = (string)$cfg['mail_from'] !== ''
    ? (string)$cfg['mail_from']
    : ('noreply@' . preg_replace('/[^a-z0-9.-]/i', '', $_SERVER['HTTP_HOST'] ?? 'localhost'));
$fromName = (string)($cfg['mail_from_name'] ?? 'Bisc8');

$headers = ['From: ' . ($fromName !== '' ? sprintf('"%s" <%s>', addcslashes($fromName, '"'), $from) : $from),
            'MIME-Version: 1.0'];

if ($wav !== null) {
    $boundary = 'b_' . bin2hex(random_bytes(8));
    $headers[] = 'Content-Type: multipart/mixed; boundary="' . $boundary . '"';
    $body  = "--{$boundary}\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Transfer-Encoding: 8bit\r\n\r\n" . $text . "\r\n";
    $body .= "--{$boundary}\r\nContent-Type: audio/wav; name=\"domanda.wav\"\r\n"
           . "Content-Transfer-Encoding: base64\r\nContent-Disposition: attachment; filename=\"domanda.wav\"\r\n\r\n"
           . chunk_split(base64_encode($wav)) . "\r\n";
    $body .= "--{$boundary}--\r\n";
} else {
    $headers[] = 'Content-Type: text/plain; charset=utf-8';
    $headers[] = 'Content-Transfer-Encoding: 8bit';
    $body = $text;
}

$ok = @mail($to, $subject, $body, implode("\r\n", $headers));
if ($ok) {
    echo json_encode(['ok' => true, 'attached' => $wav !== null]);
} else {
    http_response_code(502);
    echo json_encode(['ok' => false, 'error' => 'mail() failed']);
}
