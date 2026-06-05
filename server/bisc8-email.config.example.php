<?php
// Bisc8 email relay configuration.
//
// 1. Copy this file to `bisc8-email.config.php` (same folder, next to
//    bisc8-email.php). That copy is git-ignored, so your token never lands in
//    the repo.
// 2. Fill in the values below.
//
// (This is a .php file returning an array: even if requested directly over HTTP
// it executes and prints nothing, so the token is not served as plain text.)

return [
    // Shared secret. Must match the "Relay token" you set in the Bisc8 captive
    // portal. Generate a long random value, e.g.:  openssl rand -hex 24
    'token' => '',

    // Where the oracle responses are emailed. Fixing it here means a leaked
    // token can only ever mail YOU, never an arbitrary address.
    'mail_to' => 'you@example.com',

    // Optional. Defaults to noreply@<your-host>.
    'mail_from' => '',
    'mail_from_name' => 'Bisc8',

    // Local timezone for the email's spelled-out date/time (e.g. "Giovedì 5
    // giugno 2026, 18:42"). Defaults to Europe/Rome if omitted.
    'timezone' => 'Europe/Rome',

    // Reject attachments larger than this (the question WAV is well under 1 MB).
    'max_attach_mb' => 9,
];
