# Bisc8 email relay

A tiny, **standalone** PHP endpoint that turns a Bisc8 voice query into an email
(transcript + oracle answer + the original `question.wav` attached).

The device cannot send email itself: it has no mail server, and its residential
IP would be rejected by Gmail & co. So it POSTs here, and this script sends the
mail from a real web host. No email credentials ever live on the device — only a
shared relay token.

## Files

- `bisc8-email.php` — the endpoint. No framework, no secrets. Deploy as-is.
- `bisc8-email.config.example.php` — copy to `bisc8-email.config.php` and fill in.
- `bisc8-email.config.php` — **your** config (token + recipient). Git-ignored.

## Deploy

1. Upload `bisc8-email.php` to any PHP host folder, e.g.
   `https://netmi.lk/nomenomen/api/bisc8-email.php`
2. Copy `bisc8-email.config.example.php` → `bisc8-email.config.php` in the same
   folder and set:
   - `token` — a long random string (`openssl rand -hex 24`)
   - `mail_to` — where responses go (e.g. your Gmail)
   (Alternatively set env vars `BISC8_RELAY_TOKEN` / `BISC8_MAIL_TO`; they
   override the file.)
3. Open the URL in a browser. You should see:
   `{"ok":true,"service":"bisc8-email","ready":true}` (`ready:true` = configured).

## Point the device at it

In the Bisc8 captive portal, **Email** section:

- enable "send responses"
- **Relay URL** = the full URL to the file, e.g.
  `https://netmi.lk/nomenomen/api/bisc8-email.php` (this is not a secret)
- **Relay token** = the same `token` from your config (this *is* the secret)

Save + reboot, then ask the oracle a question.

## How it sends mail

By default it uses PHP `mail()`, which hands off to the host's own mail server
(the same way most PHP apps send mail with no SMTP auth). If your host needs an
explicit SMTP relay instead, swap the `mail()` call at the bottom for your SMTP
client — the rest (auth, MIME, attachment) stays the same.

## Notes

- The recipient is fixed server-side (`mail_to`), so a leaked token can only ever
  email you, never an arbitrary address.
- The token is compared with `hash_equals` (constant-time).
- This relay is intentionally generic: today it can live alongside another app
  (e.g. Nomen Omen), tomorrow it can be a dedicated Bisc8 relay — the device does
  not change, only the Relay URL/token it points at.
