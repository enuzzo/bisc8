#!/usr/bin/env python3
"""Embed the captive-portal HTML into web_portal.cpp.

Source of truth is ``main/web_portal.html`` (editable, with two placeholder
tokens). This tool base64-inlines the offline assets (the Pixelify Sans woff2
and the cracker mascot PNG) and splices the result into the ``kIndexHtml`` raw
string literal in ``web_portal.cpp``. The captive portal is served with no
internet access, so fonts/images must be embedded rather than linked.

Run after editing web_portal.html:  python3 tools/embed_portal.py
"""
import base64
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/bisc8_fortune/main"
TEMPLATE = MAIN / "web_portal.html"
CPP = MAIN / "web_portal.cpp"
FONT = ROOT / "docs/fonts/ChiKareGo2.woff2"      # __FONT_B64__  -> family 'ChiKareGo2' (titles/labels)
PIXOLDE = ROOT / "docs/fonts/Pixolde.woff2"      # __PIXOLDE_B64__ -> family 'Pixolde' (body + accent fallback)
MASCOT = ROOT / "assets/logo/logo_min.png"
LOGO = ROOT / "assets/logo/bisc8-logo-portal.png"  # __LOGO_B64__ -> hero logo at the top of the portal
PREVIEW = pathlib.Path("/tmp/portal_preview.html")

START = 'kIndexHtml = R"HTML('
END = ')HTML";'


def b64(path: pathlib.Path) -> str:
    return base64.b64encode(path.read_bytes()).decode("ascii")


def main() -> None:
    html = TEMPLATE.read_text(encoding="utf-8")
    html = (html.replace("__FONT_B64__", b64(FONT))
                .replace("__PIXOLDE_B64__", b64(PIXOLDE))
                .replace("__MASCOT_B64__", b64(MASCOT))
                .replace("__LOGO_B64__", b64(LOGO)))

    for tok in ("__FONT_B64__", "__PIXOLDE_B64__", "__MASCOT_B64__", "__LOGO_B64__"):
        assert tok not in html, f"token left unfilled: {tok}"
    assert END not in html, "content collides with the raw-string delimiter"

    PREVIEW.write_text(html, encoding="utf-8")

    src = CPP.read_text(encoding="utf-8")
    assert src.count(START) == 1, "expected exactly one kIndexHtml literal"
    i = src.index(START) + len(START)
    j = src.index(END, i)
    new = src[:i] + html + src[j:]
    CPP.write_text(new, encoding="utf-8")
    print(f"embedded portal: web_portal.cpp now {len(new)} bytes; preview at {PREVIEW}")


if __name__ == "__main__":
    main()
