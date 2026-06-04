# Bisc8 Web Flash Page

This folder is a static public flashing page for GitHub Pages or any HTTPS host. It uses ESP Web Tools and a manifest with the ESP32-C6 ESP-IDF offsets:

- `0x0`: bootloader
- `0x8000`: partition table
- `0x10000`: application

Publish firmware artifacts into `public/flash/firmware/` before deploying the page. Public firmware must not include OpenAI API keys, email relay tokens, Wi-Fi credentials, or provider secrets.

Prepare the folder from a completed ESP-IDF build:

```sh
python3 tools/prepare_web_flash.py --build-dir /private/tmp/bisc8-fortune-build12
```

Then serve `public/flash/` over HTTPS or localhost. Web Serial browsers will reject ordinary insecure HTTP pages.
