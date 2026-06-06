#include "web_portal.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "app_config.h"
#include "build_info.h"
#include "connectivity_service.h"
#include "debug_serial.h"
#include "display_service.h"

namespace bisc8 {

namespace {

constexpr size_t kMaxFormBodyBytes = 4096;
constexpr uint16_t kMaxScanRecords = 20;
using FormFields = std::vector<std::pair<std::string, std::string>>;

const char *const kPortalRoutes[] = {
    "/",
    "/api/status",
    "/api/wifi/scan",
    "/api/wifi/credentials",
    "/api/settings",
    "/api/openai",
    "/api/email",
    "/api/smtp",
    "/api/reset",
    "/api/reboot",
};

const char *const kCaptiveProbePaths[] = {
    "/generate_204",
    "/hotspot-detect.html",
    "/ncsi.txt",
    "/connecttest.txt",
};

const char *const kIndexHtml = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Bisc8</title>
<style>
/* ChiKareGo2 (titles/labels/buttons) — woff2 placeholder filled by tool. Falls back to Pixolde for ù ú ü ' … */
@font-face{font-family:'ChiKareGo2';font-style:normal;font-weight:400 700;font-display:swap;src:url(data:font/woff2;base64,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) format('woff2');}
/* Pixolde (body/hints/values) — woff2 placeholder filled by tool */
@font-face{font-family:'Pixolde';font-weight:400 700;font-display:swap;src:url(data:font/woff2;base64,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) format('woff2');}
/* MULTI-PASTEL "BISC8 OS" palette: ink on pale, varied soft tints per functional area */
:root{
  --ink:#23211c;          /* text / borders */
  --paper:#fbf6ef;        /* cream panels */
  --desk:#f0d9d2;          /* peachy-rose desktop */
  --desk-b:#e7c9c0;        /* desktop pinstripe partner */
  --rose:#f0b8bb;          /* primary action / active segment */
  --mint:#bfe0cf;          /* online dot / success / secondary */
  --butter:#f4e4a0;        /* flash / highlights */
  --lilac:#cdc6ec;         /* lingua / links / info */
  --peach:#f6cfa6;         /* section labels / tabs / badges */
  --sky:#bcd9e6;           /* extra accent */
  --shadow:rgba(0,0,0,.30);
  /* stacks per DESIGN v2 */
  --title:'ChiKareGo2','Pixolde',monospace;
  --body:'Pixolde',monospace;
}
*{box-sizing:border-box;}
html,body{margin:0;}
body{
  min-height:100vh;
  background:
    radial-gradient(120% 90% at 18% 0%, #f6e3da 0%, transparent 55%),
    linear-gradient(155deg, var(--desk) 0%, var(--desk-b) 100%);
  font-family:var(--body);color:var(--ink);padding:14px 10px 96px;
  -webkit-font-smoothing:none;font-smooth:never;}
.mascot{background:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAABgCAAAAADH8yjkAAAEtklEQVR4nNVa25IbKwxUb/n/f7nPA+iKYBhXNpVDtuxlLCS17jgLyu+un1/m//sC/r1Feec13DP+5pTI5wv2r9adkyd/VfwFgAtaUx62+5MCKCKYXClQefciHkxEexEhoKxx75OzAIopC4FQg5TgrYhHJ6sxOIXo7gzCPzwK4LB8PGMneXJDEP5Ahn0GYCuCke0BwZn/0dNB9F7AE38SfV3Kz7YCYoBuSToJ5cnJyTfZ1NopHjwWu3gYsgYHRdj5mhc+YFYOLRqICEHhIe0OoZbJelPkSmgnnxFc8d/HQPjgsulvWHFH4NveRArzvmgGPqE7ETsfUCDECwmRTwq2Qya/6VtLRNtvvQAKZFMIXixvUK4CbAMRHmvRZEJ93QCeCDRT2EINmBHfLBkXc5aGE9mm/qIbn1cypN0cU2rKY1ZNIqwSfb+LicbJlBq9pAgQo5bpbZS9Tn+RTxPrXX2sUCtqlkyw3a5cL3Xeo6xLwDIah9NtJO7CE6LTXUMRi8VTNd1ENYSUUfynDSBQfyERPggo5NCcTD6Y3YZr0yzJtG+q0+xziltzoEa7V4UnEw1buJg81Hk5MXpEtclsoj5BMGI/PjD+NX8hQjptZtiHKUFoSa3MbCCjXRxkUi/QNgLUNGEigTvEWVj4MKVWhvCzHyxAz08mvxtQxQtiN6n+VIkiGLzHvw3wyXhgAwn3bCH8TF0n+a5xbNvnjE96YyikwwegdiUMcoeAZPZmDQfT7qDMN4ePYQ0KRSOdLyFjdPRugCEiSPhJ2oWMUgkaiVv9iZFa7GEumTzD3LwL8oH/tDvGT422EEUQTXjCvDJh74xkdtRzM5rKHQ0Si4lHdLi0bgAYfy9QmxvOwmFUUGWwAyCzwSn6ldR8ADPUNusbBChFbm5rmEZmpoYG6QmEhr9/vDirRhENSsjskQ0L9zqA+7wXqXrVZEa4uIqbLs88c5UL1CIg0c5y1xZS0XkPUUJjyqUfBGKzfusCWPgGI62URQBSl5lucFaLLsxMm3wsTmakGmV4VNbmskVQAEj8cuqQB6q0AOFePS3cfMEFGaUwZVfT1oqJZvfOIER7xIJgdv9TSWkR2PHx3k1v8MkjGLVxVE000x7GFboLRWHwn9ocnVwXRThdTXvxp0IOD00SprC44C8USjiV+DMLSCoM4RcClNPsPENHRrnUT024CbjhbwolM5nVHBHDPvbyOwhm8cCffvMPViSDStcS3KOBk7EwGEH9h9lpQWCQGdyQvOkucVyXApIPisGSd6KZXgAIUtT4EsJpqBpTwx19ayK3RzRztJDKd5Lw6ZXynsz0+KELoP2IhLdbABIZJgApo0x1R/pGhAX/1DXEy0AWIrUgfOBe6lDMXiMKiEzIHQi3aTJzUS+VkJCIjxJifia168FkdGrWq0sO/D3cGSwzVvniTbNrKR5FRuxGOgjpKDWnqoU4Xhnt8XhIlEkvXQH0KqE3Lxb+4Yqe7QY/n1prQkDxOTHg6PAudg43kGV1z6ceTANDwVufbi9B7ePtMKimyVM7sef/dIXpXCDM5/jaRJG/eyGNwEXu4ab7JCCFQaIOgg8AHk3UpMG7dXGI33P/G+vFf55/t/7/fzzzH8QdeELy6qQhAAAAAElFTkSuQmCC) center/contain no-repeat;display:inline-block;image-rendering:pixelated;}
.app{max-width:390px;margin:0 auto;background:var(--paper);border:2px solid var(--ink);box-shadow:4px 4px 0 var(--shadow);}
/* System-7 title bar: horizontal pinstripes + square close box + centered ChiKareGo2 title, NO mascot */
.titlebar{position:relative;height:28px;border-bottom:2px solid var(--ink);
          background:repeating-linear-gradient(var(--ink) 0 1px,var(--paper) 1px 3px);display:flex;align-items:center;justify-content:center;}
.close{position:absolute;left:8px;top:7px;width:13px;height:13px;border:2px solid var(--ink);background:var(--paper);}
.close:active{background:var(--ink);}
.close-r{left:auto;right:8px;}
.copy{font-family:var(--body);font-size:11px;text-align:center;color:#6b6055;margin:16px 0 2px;letter-spacing:.5px;line-height:1.5;}
.copy a{color:var(--ink);}
.ttl{background:var(--paper);padding:0 14px;font-family:var(--title);font-size:18px;font-weight:400;letter-spacing:.5px;display:flex;align-items:center;}
.wrap{padding:14px 13px 16px;}
/* 2x2 status grid — each cell a subtly different tint */
.status{display:grid;grid-template-columns:1fr 1fr;border:2px solid var(--ink);margin-bottom:16px;}
.status .cell{padding:8px 10px;border-top:2px solid var(--ink);border-left:2px solid var(--ink);}
.status .cell:nth-child(1){background:var(--mint);}
.status .cell:nth-child(2){background:var(--sky);}
.status .cell:nth-child(3){background:var(--peach);}
.status .cell:nth-child(4){background:var(--lilac);}
.status .cell:nth-child(1),.status .cell:nth-child(2){border-top:none;}
.status .cell:nth-child(odd){border-left:none;}
.status .k{font-family:var(--title);font-size:11px;letter-spacing:1px;text-transform:uppercase;}
.status .v{font-family:var(--body);font-size:15px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;margin-top:1px;}
.dot-on{display:inline-block;width:8px;height:8px;background:var(--mint);border:1px solid var(--ink);margin-right:5px;vertical-align:1px;}
b{font-weight:400;}
/* bordered group: 2px border, notched ChiKareGo2 uppercase label on a tinted chip */
.group{position:relative;border:2px solid var(--ink);padding:18px 12px 14px;margin:0 0 16px;background:var(--paper);}
.group>.glabel{position:absolute;top:-11px;left:11px;background:var(--peach);border:2px solid var(--ink);padding:1px 8px;font-family:var(--title);font-size:13px;font-weight:400;letter-spacing:2px;text-transform:uppercase;}
/* group divs are the 2nd..6th div children of .wrap (status div is 1st): Wi-Fi=2 peach(default), Lingua=3, Oracolo=4, Email=5, Reset=6 */
.group:nth-of-type(3)>.glabel{background:var(--lilac);}  /* Lingua */
.group:nth-of-type(4)>.glabel{background:var(--mint);}   /* Oracolo */
.group:nth-of-type(5)>.glabel{background:var(--sky);}    /* Email */
.group:nth-of-type(6)>.glabel{background:var(--rose);}   /* Reset */
.hint{font-family:var(--body);font-size:13px;line-height:1.45;margin:0 0 11px;color:#5a5249;}
.hint.below{margin:10px 0 0;}
label.fld{display:block;font-family:var(--title);font-size:11px;letter-spacing:1px;text-transform:uppercase;margin:0 0 5px;color:#6b6055;}
input,select{font-family:var(--body);width:100%;min-height:44px;border:2px solid var(--ink);background:#fffdf7;color:var(--ink);padding:9px 11px;font-size:16px;border-radius:0;}
input::placeholder{color:#a39686;}
input:focus,select:focus{outline:none;background:var(--paper);box-shadow:3px 3px 0 var(--rose);}
.field{margin-bottom:12px;}.pwd{position:relative;display:block;}.pwd>input{padding-right:50px;}.pwd .eye{position:absolute;right:2px;top:2px;bottom:2px;width:44px;border:0;border-left:2px solid var(--ink);background:var(--lilac);color:var(--ink);cursor:pointer;display:grid;place-items:center;padding:0;}.pwd .eye.on{background:var(--ink);color:var(--paper);}
.two{display:grid;grid-template-columns:1fr 1fr;gap:9px;}
.actions{display:flex;flex-wrap:wrap;gap:11px;align-items:center;margin-top:6px;}
/* primary button: rose fill, ink text, hard offset shadow */
.btn{font-family:var(--title);font-size:15px;font-weight:400;letter-spacing:.5px;background:var(--rose);color:var(--ink);border:2px solid var(--ink);padding:11px 18px;min-height:44px;box-shadow:3px 3px 0 var(--shadow);cursor:pointer;}
.btn:hover{box-shadow:3px 3px 0 var(--butter);}
.btn:active{transform:translate(3px,3px);box-shadow:none;}
/* secondary: System-7 platinum grey */
.btn.sec{background:#e3e0d8;color:var(--ink);}
.btn.sec:hover{background:#eceae3;box-shadow:3px 3px 0 var(--mint);}
.btn:disabled{opacity:.55;cursor:wait;}
.btn:focus-visible,.seg b:focus-visible,.scan button:focus-visible,.eye:focus-visible,.close:focus-visible,details summary:focus-visible{outline:2px solid var(--ink);outline-offset:3px;}
.seg{display:inline-flex;border:2px solid var(--ink);background:var(--paper);}
.seg b{font-family:var(--title);font-weight:400;padding:11px 18px;cursor:pointer;font-size:15px;min-width:48px;min-height:44px;display:inline-flex;align-items:center;justify-content:center;text-align:center;user-select:none;}
.seg b+b{border-left:2px solid var(--ink);}
.seg b.on{background:var(--rose);color:var(--ink);}
.row{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:11px 0;border-top:2px dotted var(--ink);font-family:var(--body);font-size:15px;}
.row:first-child{border-top:none;}
.muted{font-family:var(--body);font-size:13px;color:#5a5249;}
.pill{border:2px solid var(--ink);padding:3px 9px;box-shadow:2px 2px 0 var(--shadow);font-size:13px;font-family:var(--body);background:var(--butter);}
.scan{border:2px solid var(--ink);margin:0 0 11px;max-height:120px;overflow:auto;background:#fffdf7;}
.scan .h{background:repeating-linear-gradient(var(--ink) 0 1px,var(--paper) 1px 3px);font-family:var(--title);font-size:11px;letter-spacing:1px;text-transform:uppercase;padding:4px 8px;border-bottom:2px solid var(--ink);}
.scan .h span{background:var(--sky);padding:0 5px;}
.scan button{display:block;width:100%;text-align:left;border:0;border-bottom:1px solid var(--ink);background:transparent;font-family:var(--body);font-size:14px;padding:9px 9px;min-height:42px;cursor:pointer;}
.scan button:last-child{border-bottom:0;}
.scan button:hover{background:var(--ink);color:var(--paper);}
details summary{font-family:var(--title);font-size:13px;font-weight:400;cursor:pointer;margin:6px 0 9px;letter-spacing:.5px;color:#6a5fae;}
.foot{font-family:var(--body);font-size:12px;line-height:1.45;margin-top:14px;color:#5a5249;}
.diac{margin-top:14px;border-top:2px dotted var(--ink);padding-top:10px;}
.diac .k{font-family:var(--title);font-size:10px;letter-spacing:1px;text-transform:uppercase;color:#6b6055;}
.diac p{font-family:var(--body);font-size:17px;margin:5px 0 0;line-height:1.5;}
/* reboot bar — ink fill with butter pinstripe edge */
.rebar{position:fixed;left:50%;transform:translateX(-50%);bottom:0;width:100%;max-width:390px;background:var(--ink);color:var(--paper);padding:11px 12px 12px;display:none;align-items:center;gap:11px;z-index:60;}
.rebar.show{display:flex;}
.rebar::before{content:"";position:absolute;top:-3px;left:0;right:0;height:3px;background:repeating-linear-gradient(90deg,var(--butter) 0 4px,transparent 4px 8px);}
.rebar .ic{width:30px;height:30px;flex:none;background:var(--paper);display:grid;place-items:center;}
.rebar .ic .mascot{width:24px;height:24px;}
.rebar .txt{flex:1;font-family:var(--body);font-size:14px;line-height:1.22;}
.rebar .txt b{font-family:var(--title);display:block;font-size:15px;font-weight:400;}
.rebar .go{font-family:var(--title);font-size:15px;font-weight:400;background:var(--butter);color:var(--ink);border:2px solid var(--butter);padding:11px 14px;min-height:44px;box-shadow:3px 3px 0 rgba(255,255,255,.35);cursor:pointer;flex:none;}
.toast{position:fixed;left:50%;transform:translateX(-50%);bottom:84px;background:var(--ink);color:var(--paper);padding:9px 14px;font-family:var(--body);font-size:14px;display:none;z-index:70;}
.toast.show{display:block;}
.hidden{display:none;}
/* ===== TYPOGRAPHY v5 — bigger & airier: harmonized scale + more spacing, NO letter-spacing ===== */
html{font-size:100%;}
body{font-size:1.1rem;line-height:1.55;letter-spacing:normal;}
/* slightly wider column so the bigger type has room to breathe */
.app,.rebar{max-width:26rem;}
/* hero logo at the top — cookie-wizard, hard-offset shadow, gentle float + twinkling sparkles */
.logo-stage{display:block;position:relative;width:-moz-fit-content;width:fit-content;max-width:62%;margin:.7rem auto -.1rem;}
.logo{display:block;width:10.5rem;max-width:100%;height:auto;filter:drop-shadow(.2rem .26rem 0 var(--shadow));}
.spark{position:absolute;font-family:var(--title);color:var(--ink);opacity:0;pointer-events:none;z-index:3;text-shadow:0 0 .5rem var(--butter),0 0 1rem var(--butter);}
.spark.s1{top:-.3rem;right:-.2rem;font-size:2rem;}
.spark.s2{bottom:1.6rem;left:-.5rem;font-size:1.6rem;}
.spark.s3{top:2.6rem;right:-.6rem;font-size:1.75rem;}
.spark.s4{top:.7rem;left:0;font-size:1.45rem;}
@media (prefers-reduced-motion:no-preference){
  .logo{animation:bob 4.6s ease-in-out infinite;}
  @keyframes bob{0%,100%{transform:translateY(0);}50%{transform:translateY(-.6rem);}}
  .spark.s1{animation:tw 3s ease-in-out .1s infinite;}
  .spark.s2{animation:tw 3s ease-in-out 1s infinite;}
  .spark.s3{animation:tw 3s ease-in-out 1.9s infinite;}
  .spark.s4{animation:tw 3s ease-in-out 2.5s infinite;}
  @keyframes tw{0%,100%{opacity:0;transform:scale(.5) rotate(-12deg);}45%{opacity:1;transform:scale(1.3) rotate(12deg);}}
}
.wrap{padding:1.35rem 1.15rem 1.65rem;}
/* window title = a bordered LABEL chip sitting ON the pinstripe bar (not a white hole) */
.titlebar{height:2.1rem;}
.ttl{background:var(--paper);border:.125rem solid var(--ink);border-top:0;border-bottom:0;padding:0 .65rem;font-family:var(--title);font-size:1.1rem;line-height:1.4;letter-spacing:normal;display:inline-flex;align-items:center;align-self:stretch;}
.close,.close-r{top:.55rem;}
/* status block: more air, color ONLY as a ~6px LEFT BAR (override the base per-cell full fill) */
.status{margin-bottom:1.5rem;}
.status .cell{background:var(--paper);box-shadow:inset .85rem 0 0 var(--cell-bar);padding:.4rem .85rem .45rem 1.4rem;}
.status .cell:nth-child(1){background:var(--paper);--cell-bar:var(--mint);}
.status .cell:nth-child(2){background:var(--paper);--cell-bar:var(--sky);}
.status .cell:nth-child(3){background:var(--paper);--cell-bar:var(--peach);}
.status .cell:nth-child(4){background:var(--paper);--cell-bar:var(--lilac);}
.status .k{font-size:1.35rem;letter-spacing:normal;text-transform:none;}
.status .v{font-size:1.2rem;letter-spacing:normal;margin-top:.05rem;}
/* groups: more internal air + more space between sections */
.group{padding:1.6rem 1.2rem 1.4rem;margin:0 0 1.6rem;}
.group>.glabel{top:-.85rem;font-size:1.2rem;letter-spacing:normal;text-transform:none;padding:.1rem .6rem;}
label.fld{font-size:1.1rem;letter-spacing:normal;text-transform:none;color:var(--ink);margin:0 0 .55rem;}
.field{margin-bottom:1.1rem;}
.hint{font-size:1.3rem;line-height:1.12;letter-spacing:normal;margin:.625rem 0 1.2rem;color:#5a5249;}
.hint.below{margin:1.05rem 0 0;}
input,select{font-size:1.2rem;letter-spacing:normal;min-height:2.9rem;padding:.62rem .8rem;}
/* reference button: rose fill, ink border, top bevel + hard shadow, ChiKareGo2 */
.btn{font-family:var(--title);font-size:1.2rem;letter-spacing:normal;background:var(--rose);color:var(--ink);border:.125rem solid var(--ink);padding:.78rem 1.45rem;min-height:2.9rem;box-shadow:inset 0 2px 0 rgba(255,255,255,.55),3px 3px 0 var(--shadow);}
.btn:hover{box-shadow:inset 0 2px 0 rgba(255,255,255,.55),3px 3px 0 var(--shadow);}
.btn.sec{background:#e6e3db;}
.btn:active{transform:translate(2px,2px);box-shadow:inset 0 2px 0 rgba(255,255,255,.4),1px 1px 0 var(--shadow);}
.actions{gap:.85rem;margin-top:.55rem;}
.seg b{font-size:1.2rem;letter-spacing:normal;padding:.72rem 1.25rem;}
.row{font-size:1.2rem;letter-spacing:normal;padding:.85rem 0;}
.muted{font-size:1.1rem;letter-spacing:normal;}
.pill{font-size:1.1rem;padding:.25rem .65rem;}
.foot,.copy{font-size:1.1rem;line-height:1.5;letter-spacing:normal;}
details summary{font-size:1.1rem;letter-spacing:normal;margin:.6rem 0 .85rem;}
/* networks-found: a clear solid header bar */
.scan{margin:0 0 1.05rem;max-height:9.5rem;}
.scan .h{background:var(--sky);font-family:var(--title);font-size:1.15rem;letter-spacing:normal;text-transform:none;padding:.55rem .85rem;color:var(--ink);}
.scan .h span{background:transparent;padding:0;}
.scan button{font-size:1.15rem;letter-spacing:normal;min-height:2.9rem;padding:.72rem .85rem;}
/* diacritics sample */
.diac{margin-top:1.1rem;padding-top:.85rem;}
.diac .k{font-size:1.1rem;letter-spacing:normal;}
.diac p{font-size:1.25rem;line-height:1.5;}
/* reboot bar + toast: floor at 1.1rem too (override base px) */
.rebar .txt{font-size:1.1rem;line-height:1.3;}
.rebar .txt b{font-size:1.2rem;}
.rebar .go{font-size:1.2rem;}
.toast{font-size:1.1rem;}
</style>
</head>
<body>
<div class="app">
  <div class="titlebar"><div class="close"></div><div class="ttl">Bisc8</div><div class="close close-r"></div></div>
  <div class="wrap">

    <span class="logo-stage">
      <span class="spark s1" aria-hidden="true">+</span>
      <span class="spark s2" aria-hidden="true">*</span>
      <span class="spark s3" aria-hidden="true">+</span>
      <span class="spark s4" aria-hidden="true">*</span>
      <img class="logo" src="data:image/png;base64,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" alt="Bisc8 — the cookie-wizard">
    </span>
    <p data-i18n="tagline" style="text-align:center;font-family:var(--title);font-size:1.3rem;letter-spacing:normal;text-transform:capitalize;margin:.2rem 0 1.1rem;color:var(--ink);">briciomanzia tascabile</p>

    <div class="status">
      <div class="cell"><div class="k" data-i18n="st_state">Stato</div><div class="v" data-bind="wifi_mode">setup</div></div>
      <div class="cell"><div class="k" data-i18n="st_net">Rete</div><div class="v" data-bind="network_label">setup</div></div>
      <div class="cell"><div class="k" data-i18n="st_addr">Indirizzo</div><div class="v" data-bind="device_address">192.168.4.1</div></div>
      <div class="cell"><div class="k" data-i18n="st_lang">Lingua</div><div class="v" data-bind="language">IT</div></div>
    </div>

    <div class="group">
      <span class="glabel" data-i18n="wifi_label">Wi-Fi</span>
      <p class="hint" data-i18n="wifi_hint">.</p>
      <div class="actions" style="margin-bottom:11px">
        <button class="btn sec" type="button" id="scan" data-i18n="wifi_scan">Cerca reti</button>
        <span class="muted" id="scanState" data-i18n="wifi_scan_note">.</span>
      </div>
      <div class="scan"><div class="h"><span data-i18n="scan_h">Reti trovate</span></div><div id="scanList"></div></div>
      <form data-api="/api/wifi/credentials">
        <div class="field"><label class="fld" data-i18n="ssid">SSID</label><input id="ssid" name="ssid" autocomplete="off" required></div>
        <div class="field"><label class="fld" data-i18n="password">Password</label><span class="pwd"><input name="password" type="password" autocomplete="new-password" data-i18n-placeholder="pwd_ph"><button class="eye" type="button" aria-pressed="false" aria-label="show"><svg viewBox="0 0 24 16" width="22" height="15" aria-hidden="true"><path d="M2 8 Q12 1 22 8 Q12 15 2 8 Z" fill="none" stroke="currentColor" stroke-width="2"/><circle cx="12" cy="8" r="3.1" fill="currentColor"/></svg></button></span></div>
        <div class="actions"><button class="btn" type="submit" data-i18n="save_wifi">Salva Wi-Fi</button></div>
      </form>
      <form data-api="/api/wifi/credentials" class="remove">
        <input type="hidden" name="action" value="remove">
        <div class="row" style="margin-top:11px">
          <span data-i18n="forget_label">Dimentica slot</span>
          <span style="display:flex;gap:8px;align-items:center">
            <input name="index" inputmode="numeric" placeholder="0" aria-label="slot" style="width:58px;min-height:42px;text-align:center">
            <button class="btn sec" type="submit" data-i18n="forget">Dimentica</button>
          </span>
        </div>
      </form>
    </div>

    <div class="group">
      <span class="glabel" data-i18n="lang_label">Lingua</span>
      <p class="hint" data-i18n="lang_hint">.</p>
      <div class="seg" id="langSeg"><b data-lang="it">IT</b><b data-lang="en">EN</b><b data-lang="es">ES</b></div>
    </div>

    <div class="group">
      <span class="glabel" data-i18n="oracle_label">Oracolo</span>
      <p class="hint" data-i18n="oracle_hint">.</p>
      <form data-api="/api/openai">
        <div class="field"><label class="fld" data-i18n="api_key">Chiave API</label><span class="pwd"><input name="api_key" type="password" data-i18n-placeholder="keep_key"><button class="eye" type="button" aria-pressed="false" aria-label="show"><svg viewBox="0 0 24 16" width="22" height="15" aria-hidden="true"><path d="M2 8 Q12 1 22 8 Q12 15 2 8 Z" fill="none" stroke="currentColor" stroke-width="2"/><circle cx="12" cy="8" r="3.1" fill="currentColor"/></svg></button></span></div>
        <div class="two">
          <div class="field"><label class="fld" data-i18n="stt">Voce a testo</label><input name="transcription_model" placeholder="whisper-1"></div>
          <div class="field"><label class="fld" data-i18n="model">Modello oracolo</label><input name="response_model" placeholder="gpt-5.4-mini"></div>
        </div>
        <div class="two">
          <div class="field"><label class="fld" data-i18n="tts">Testo a voce</label><input name="speech_model" placeholder="tts-1-hd"></div>
          <div class="field"><label class="fld" data-i18n="voice">Voce</label><input name="voice" placeholder="alloy"></div>
        </div>
        <div class="actions"><button class="btn" type="submit" data-i18n="save_oracle">Salva Oracolo</button></div>
      </form>
      <p class="hint below"><span data-i18n="key_stored">Chiave salvata:</span> <span data-bind="openai_key">missing</span></p>
    </div>

    <div class="group">
      <span class="glabel" data-i18n="email_label">Email</span>
      <form data-api="/api/email">
        <div class="field"><label class="fld" data-i18n="recipient">Destinatario</label><input name="recipient" type="email" data-i18n-placeholder="recipient_ph"></div>
        <div class="row">
          <span data-i18n="send_resp">Manda i responsi</span>
          <span class="seg" id="emailSeg"><b data-val="1" data-i18n="yes">Si</b><b data-val="0" data-i18n="no">No</b></span>
          <input type="hidden" name="enabled" id="emailEnabled" value="0">
        </div>
        <details>
          <summary><span data-i18n="relay_adv">Relay (avanzato)</span></summary>
          <div class="field"><label class="fld" data-i18n="relay_url">URL relay</label><input name="relay_url" inputmode="url" placeholder="https://relay.example.com/v1/bisc8/email"></div>
          <div class="field"><label class="fld" data-i18n="relay_token">Token relay</label><span class="pwd"><input name="relay_token" type="password" data-i18n-placeholder="keep_token"><button class="eye" type="button" aria-pressed="false" aria-label="show"><svg viewBox="0 0 24 16" width="22" height="15" aria-hidden="true"><path d="M2 8 Q12 1 22 8 Q12 15 2 8 Z" fill="none" stroke="currentColor" stroke-width="2"/><circle cx="12" cy="8" r="3.1" fill="currentColor"/></svg></button></span></div>
        </details>
        <div class="actions"><button class="btn" type="submit" data-i18n="save_email">Salva Email</button></div>
      </form>
      <p class="hint below"><span data-i18n="rec_lbl">Destinatario</span>: <span data-bind="email_recipient">missing</span>. <span data-i18n="relay_lbl">Relay</span>: <span data-bind="email_relay">missing</span></p>
    </div>

    <div class="group">
      <span class="glabel" data-i18n="reset_label">Reset</span>
      <div class="actions" style="margin-bottom:9px"><button class="btn sec" type="button" id="reset" data-i18n="reset_btn">Reset totale</button></div>
      <p class="hint" style="margin:0" data-i18n="reset_hint">.</p>
    </div>

    <div class="row"><span data-i18n="firmware">Firmware</span><span class="pill" data-bind="build_version">r----</span></div>
    <p class="foot" data-i18n="secret_warn">.</p>

    <div class="diac">
      <div class="k" data-i18n="diac_label">test</div>
      <p>&agrave;&egrave;&eacute;&igrave;&ograve;&ugrave; &aacute;&iacute;&oacute;&uacute; &ntilde; &uuml; &ccedil; &iquest;&iexcl;</p>
    </div>
    <p class="copy">&copy; enuzzo &middot; Netmilk Studio &middot; <a href="https://github.com/enuzzo/bisc8" target="_blank" rel="noopener">github.com/enuzzo/bisc8</a></p>
  </div>
</div>

<div class="rebar" id="rebar">
  <div class="ic"><span class="mascot"></span></div>
  <div class="txt"><b data-i18n="reboot_title">Wi-Fi pronta.</b><span data-i18n="reboot_msg">Testata e salvata. Riavvia per applicarla.</span></div>
  <button class="go" type="button" id="reboot" data-i18n="reboot_now">Riavvia ora</button>
</div>
<div class="toast" id="toast"></div>

<script>
const I18N={
 it:{st_state:"Stato",st_net:"Rete",st_addr:"Indirizzo",st_lang:"Lingua",tagline:"briciomanzia tascabile",
 wifi_label:"Wi-Fi",wifi_hint:"Le reti salvate restano qui, sul biscotto. Niente cloud, niente telefonate a casa.",
 wifi_scan:"Cerca reti",wifi_scan_note:"le reti a portata d'orecchio, niente di più.",scan_h:"Reti trovate",
 ssid:"SSID",password:"Password",pwd_ph:"quella che sai tu",save_wifi:"Salva Wi-Fi",forget_label:"Dimentica slot",forget:"Dimentica",
 lang_label:"Lingua",lang_hint:"La lingua in cui vaticina l'oracolo. Scegline una: i responsi seguono.",
 oracle_label:"Oracolo",oracle_hint:"La chiave resta sul biscotto e da nessun'altra parte. Acqua in bocca.",
 api_key:"Chiave API",keep_key:"lascia vuoto per non toccarla",stt:"Voce a testo",model:"Modello oracolo",tts:"Testo a voce",voice:"Voce",
 save_oracle:"Salva Oracolo",key_stored:"Chiave salvata:",
 email_label:"Email",recipient:"Destinatario",recipient_ph:"tu@esempio.it",send_resp:"Manda i responsi",yes:"Sì",no:"No",
 relay_adv:"Relay (avanzato)",relay_url:"URL relay",relay_token:"Token relay",keep_token:"lascia vuoto per non toccarlo",save_email:"Salva Email",
 rec_lbl:"Destinatario",relay_lbl:"Relay",
 reset_label:"Reset",reset_btn:"Reset totale",reset_hint:"Cancella Wi-Fi, oracolo, email e lingua. Di nuovo un biscotto vuoto e a digiuno.",
 firmware:"Firmware",secret_warn:"I segreti vivono in chiaro su questo biscotto. Attiva la flash encryption prima di spedirne uno.",
 reboot_title:"Wi-Fi dentro.",reboot_msg:"Testata, salvata, pronta. Riavvia per renderla effettiva.",reboot_now:"Riavvia ora",
 diac_label:"test diacritici",
 missing:"manca",setupMode:"setup",onlineMode:"connesso",offlineMode:"offline",configured:"configurato",
 saved:"Salvato",rebooting:"Riavvio...",scanning:"Scansiono...",networksFound:"reti trovate",
 resetConfirm:"Cancellare tutta la configurazione locale?",resetDone:"Configurazione cancellata"},
 en:{st_state:"Status",st_net:"Network",st_addr:"Address",st_lang:"Language",tagline:"pocket crumbomancy",
 wifi_label:"Wi-Fi",wifi_hint:"Saved networks live here on the biscuit. Nothing leaves, nothing phones home.",
 wifi_scan:"Scan",wifi_scan_note:"the networks within earshot, nothing more.",scan_h:"Networks found",
 ssid:"SSID",password:"Password",pwd_ph:"the one you know",save_wifi:"Save Wi-Fi",forget_label:"Forget slot",forget:"Forget",
 lang_label:"Language",lang_hint:"The tongue the oracle prophesies in. Pick one — the readings follow.",
 oracle_label:"Oracle",oracle_hint:"Your key stays on the biscuit and nowhere else. We don't kiss and tell.",
 api_key:"API key",keep_key:"leave blank to keep it",stt:"Speech to text",model:"Oracle model",tts:"Text to speech",voice:"Voice",
 save_oracle:"Save Oracle",key_stored:"Stored key:",
 email_label:"Email",recipient:"Recipient",recipient_ph:"you@example.com",send_resp:"Send the readings",yes:"Yes",no:"No",
 relay_adv:"Relay (advanced)",relay_url:"Relay URL",relay_token:"Relay token",keep_token:"leave blank to keep it",save_email:"Save Email",
 rec_lbl:"Recipient",relay_lbl:"Relay",
 reset_label:"Reset",reset_btn:"Full reset",reset_hint:"Wipes Wi-Fi, oracle, email and language. Back to a blank, hungry biscuit.",
 firmware:"Firmware",secret_warn:"Secrets live on this biscuit, in the clear. Turn on flash encryption before you ship one.",
 reboot_title:"Wi-Fi's in.",reboot_msg:"Tested, saved, ready. Reboot to make it count.",reboot_now:"Reboot now",
 diac_label:"diacritics test",
 missing:"missing",setupMode:"setup",onlineMode:"online",offlineMode:"offline",configured:"set",
 saved:"Saved",rebooting:"Rebooting...",scanning:"Scanning...",networksFound:"found",
 resetConfirm:"Wipe all local configuration?",resetDone:"Configuration wiped"},
 es:{st_state:"Estado",st_net:"Red",st_addr:"Dirección",st_lang:"Idioma",tagline:"migamancia de bolsillo",
 wifi_label:"Wi-Fi",wifi_hint:"Las redes guardadas viven aquí, en la galleta. Nada sale, nada llama a casa.",
 wifi_scan:"Buscar redes",wifi_scan_note:"las redes al alcance del oído, nada más.",scan_h:"Redes encontradas",
 ssid:"SSID",password:"Contraseña",pwd_ph:"la que ya sabes",save_wifi:"Guardar Wi-Fi",forget_label:"Olvidar ranura",forget:"Olvidar",
 lang_label:"Idioma",lang_hint:"La lengua en que vaticina el oráculo. Elige una: las lecturas la siguen.",
 oracle_label:"Oráculo",oracle_hint:"Tu clave se queda en la galleta y en ningún otro sitio. Boca sellada.",
 api_key:"Clave API",keep_key:"déjalo vacío para no tocarla",stt:"Voz a texto",model:"Modelo del oráculo",tts:"Texto a voz",voice:"Voz",
 save_oracle:"Guardar Oráculo",key_stored:"Clave guardada:",
 email_label:"Email",recipient:"Destinatario",recipient_ph:"tu@ejemplo.es",send_resp:"Manda las lecturas",yes:"Sí",no:"No",
 relay_adv:"Relay (avanzado)",relay_url:"URL del relay",relay_token:"Token del relay",keep_token:"déjalo vacío para no tocarlo",save_email:"Guardar Email",
 rec_lbl:"Destinatario",relay_lbl:"Relay",
 reset_label:"Reset",reset_btn:"Reset total",reset_hint:"Borra Wi-Fi, oráculo, email e idioma. De vuelta a una galleta vacía y hambrienta.",
 firmware:"Firmware",secret_warn:"Los secretos viven en claro en esta galleta. Activa el cifrado de flash antes de enviar una.",
 reboot_title:"Wi-Fi dentro.",reboot_msg:"Probada, guardada, lista. Reinicia para que cuente.",reboot_now:"Reiniciar ahora",
 diac_label:"test de diacríticos",
 missing:"falta",setupMode:"setup",onlineMode:"conectado",offlineMode:"offline",configured:"configurado",
 saved:"Guardado",rebooting:"Reiniciando...",scanning:"Buscando...",networksFound:"encontradas",
 resetConfirm:"¿Borrar toda la configuración local?",resetDone:"Configuración borrada"}
};
let currentLanguage='en';
const $=function(s){return document.querySelector(s)};
const toast=function(){return document.getElementById('toast')}();
function tr(k){return (I18N[currentLanguage]&&I18N[currentLanguage][k])||I18N.en[k]||k}
function note(t){toast.textContent=tr(t)||t;toast.classList.add('show');setTimeout(function(){toast.classList.remove('show')},2600)}
async function api(u,o){const r=await fetch(u,o);const j=await r.json();if(!r.ok)throw new Error(j.error||'Request failed');return j}
function body(f){return new URLSearchParams(new FormData(f)).toString()}
function applyLanguage(l){currentLanguage=I18N[l]?l:'en';document.documentElement.lang=currentLanguage;
 document.querySelectorAll('[data-i18n]').forEach(function(e){e.textContent=tr(e.dataset.i18n)});
 document.querySelectorAll('[data-i18n-placeholder]').forEach(function(e){e.placeholder=tr(e.dataset.i18nPlaceholder)});
 document.querySelectorAll('#langSeg b').forEach(function(b){var on=b.dataset.lang===currentLanguage;b.classList.toggle('on',on);b.setAttribute('aria-pressed',on)});}
function valueText(k,v){if(k==='language')return (v||'').toUpperCase();
 if(v===''||v==null||v==='missing')return tr('missing');
 if(v==='setup mode')return tr('setupMode');if(v==='online')return tr('onlineMode');if(v==='offline')return tr('offlineMode');
 if(v==='configured')return tr('configured');return v;}
function setEmailEnabled(on){document.getElementById('emailEnabled').value=on?'1':'0';
 document.querySelectorAll('#emailSeg b').forEach(function(b){var sel=(b.dataset.val==='1')===!!on;b.classList.toggle('on',sel);b.setAttribute('aria-pressed',sel)});}
function fill(s){if(s.language)applyLanguage(s.language);
 for(const k in s){document.querySelectorAll('[data-bind="'+k+'"]').forEach(function(el){el.textContent=valueText(k,s[k])})}
 var _wm=document.querySelector('[data-bind="wifi_mode"]');if(_wm&&s.wifi_mode==='online'){_wm.innerHTML='<span class="dot-on"></span>'+_wm.textContent;}
 if(typeof s.email_enabled!=='undefined')setEmailEnabled(s.email_enabled);
 document.getElementById('rebar').classList.toggle('show',!!s.reboot_required);}
async function refresh(){try{fill(await api('/api/status'))}catch(e){note(e.message)}}
document.querySelectorAll('form[data-api]').forEach(function(form){form.addEventListener('submit',async function(e){e.preventDefault();const b=form.querySelector('button[type=submit]')||form.querySelector('button');if(b)b.disabled=true;try{fill(await api(form.dataset.api,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body(form)}));note('saved')}catch(err){note(err.message)}finally{if(b)b.disabled=false}})});
document.querySelectorAll('#langSeg b').forEach(function(b){b.addEventListener('click',async function(){try{fill(await api('/api/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'language='+b.dataset.lang}));note('saved')}catch(e){note(e.message)}})});
document.querySelectorAll('#emailSeg b').forEach(function(b){b.addEventListener('click',function(){setEmailEnabled(b.dataset.val==='1')})});
document.getElementById('scan').addEventListener('click',async function(){const st=document.getElementById('scanState');const list=document.getElementById('scanList');st.textContent=tr('scanning');list.textContent='';try{const j=await api('/api/wifi/scan');st.textContent=j.networks.length+' '+tr('networksFound');j.networks.forEach(function(n){const x=document.createElement('button');x.type='button';x.textContent=n.ssid+' · '+n.band+' · '+n.rssi;x.onclick=function(){document.getElementById('ssid').value=n.ssid};list.appendChild(x)})}catch(e){st.textContent=e.message}});
document.getElementById('reboot').addEventListener('click',async function(){try{await api('/api/reboot',{method:'POST'});note('rebooting')}catch(e){note(e.message)}});
document.getElementById('reset').addEventListener('click',async function(){if(!confirm(tr('resetConfirm')))return;try{fill(await api('/api/reset',{method:'POST'}));note('resetDone')}catch(e){note(e.message)}});
document.querySelectorAll('.eye').forEach(function(b){b.addEventListener('click',function(){var inp=b.parentElement.querySelector('input');var show=inp.type==='password';inp.type=show?'text':'password';b.classList.toggle('on',show);b.setAttribute('aria-pressed',show?'true':'false');b.setAttribute('aria-label',show?'hide':'show');});});
document.querySelectorAll('.seg b').forEach(function(b){b.tabIndex=0;b.setAttribute('role','button');b.addEventListener('keydown',function(e){if(e.key==='Enter'||e.key===' '){e.preventDefault();b.click();}});});
applyLanguage('en');refresh();
</script>
</body>
</html>
)HTML";

WebPortal *PortalFromRequest(httpd_req_t *req) {
    return static_cast<WebPortal *>(req->user_ctx);
}

std::string JsonString(const std::string &value) {
    std::string out = "\"";
    char escape[7] = {};
    for (unsigned char ch : value) {
        if (ch == '"' || ch == '\\') {
            out.push_back('\\');
            out.push_back(static_cast<char>(ch));
        } else if (ch == '\b') {
            out += "\\b";
        } else if (ch == '\f') {
            out += "\\f";
        } else if (ch == '\n') {
            out += "\\n";
        } else if (ch == '\r') {
            out += "\\r";
        } else if (ch == '\t') {
            out += "\\t";
        } else if (ch < 0x20) {
            snprintf(escape, sizeof(escape), "\\u%04x", ch);
            out += escape;
        } else {
            out.push_back(static_cast<char>(ch));
        }
    }
    out.push_back('"');
    return out;
}

const char *WifiBandLabel(uint8_t channel) {
    if (channel >= 1 && channel <= 14) {
        return "2.4 GHz";
    }
    if (channel >= 36) {
        return "5 GHz";
    }
    return "unknown";
}

int HexValue(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + ch - 'a';
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + ch - 'A';
    }
    return -1;
}

std::string UrlDecode(const std::string &value) {
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            out.push_back(' ');
        } else if (value[i] == '%' && i + 2 < value.size()) {
            const int hi = HexValue(value[i + 1]);
            const int lo = HexValue(value[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
            }
        } else {
            out.push_back(value[i]);
        }
    }
    return out;
}

FormFields ParseForm(const std::string &body) {
    FormFields fields;
    size_t start = 0;
    while (start <= body.size()) {
        const size_t end = body.find('&', start);
        const std::string part = body.substr(start, end == std::string::npos ? std::string::npos : end - start);
        const size_t eq = part.find('=');
        if (eq != std::string::npos) {
            fields.emplace_back(UrlDecode(part.substr(0, eq)), UrlDecode(part.substr(eq + 1)));
        } else if (!part.empty()) {
            fields.emplace_back(UrlDecode(part), "");
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return fields;
}

std::string FormValue(const FormFields &fields, const char *key, const std::string &fallback = "") {
    for (const auto &field : fields) {
        if (field.first == key) {
            return field.second;
        }
    }
    return fallback;
}

esp_err_t ReadRequestBody(httpd_req_t *req, std::string *body) {
    if (req == nullptr || body == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (req->content_len > kMaxFormBodyBytes) {
        return ESP_ERR_INVALID_SIZE;
    }

    body->clear();
    body->resize(req->content_len);
    size_t received = 0;
    while (received < req->content_len) {
        const int ret = httpd_req_recv(req, &(*body)[received], req->content_len - received);
        if (ret <= 0) {
            return ESP_FAIL;
        }
        received += static_cast<size_t>(ret);
    }
    return ESP_OK;
}

esp_err_t SendJson(httpd_req_t *req, const std::string &json) {
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json.c_str(), json.size());
}

esp_err_t SendError(httpd_req_t *req, const char *status, const char *message) {
    httpd_resp_set_status(req, status);
    return SendJson(req, std::string("{\"error\":") + JsonString(message) + "}");
}

void ApplyDefaultSettings(DeviceSettings *settings) {
    if (settings == nullptr) {
        return;
    }
    *settings = DeviceSettings{};
    settings->language = "en";
    settings->openai = DefaultOpenAiSettings();
}

void RestartTask(void *arg) {
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(400));
    DebugSerial::LogAlways("[WEB]", "rebooting after Wi-Fi apply request");
    esp_restart();
}

}  // namespace

const char *const *PortalRoutes(size_t *count) {
    if (count != nullptr) {
        *count = sizeof(kPortalRoutes) / sizeof(kPortalRoutes[0]);
    }
    return kPortalRoutes;
}

const char *const *CaptivePortalProbePaths(size_t *count) {
    if (count != nullptr) {
        *count = sizeof(kCaptiveProbePaths) / sizeof(kCaptiveProbePaths[0]);
    }
    return kCaptiveProbePaths;
}

const char *PortalIndexHtml() {
    return kIndexHtml;
}

void WebPortal::BindStatus(const WifiStatus *wifi) {
    wifi_ = wifi;
}

void WebPortal::BindConfig(ConfigStore *config_store, DeviceSettings *settings) {
    config_store_ = config_store;
    settings_ = settings;
}

void WebPortal::BindRuntime(ConnectivityService *connectivity, DisplayService *display) {
    connectivity_ = connectivity;
    display_ = display;
}

esp_err_t WebPortal::RegisterRoute(const char *uri, httpd_method_t method, HttpHandler handler) {
    httpd_uri_t route = {};
    route.uri = uri;
    route.method = method;
    route.handler = handler;
    route.user_ctx = this;
    return httpd_register_uri_handler(server_, &route);
}

esp_err_t WebPortal::HandleIndex(httpd_req_t *req) {
    DebugSerial::Log("[WEB]", "GET / free_heap=%u", static_cast<unsigned>(esp_get_free_heap_size()));
    WebPortal *portal = PortalFromRequest(req);
    if (portal != nullptr) {
        portal->activity_ = true;
    }
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    return httpd_resp_send(req, kIndexHtml, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebPortal::HandleStatus(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr) {
        return SendError(req, "500 Internal Server Error", "Portal is not ready");
    }
    portal->activity_ = true;  // the open config page heartbeats here; stay awake
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleWifiScan(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr) {
        return SendError(req, "500 Internal Server Error", "Portal is not ready");
    }
    return portal->SendWifiScanJson(req);
}

esp_err_t WebPortal::HandleWifiCredentials(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr || portal->settings_ == nullptr) {
        return SendError(req, "500 Internal Server Error", "Configuration is not ready");
    }

    std::string body;
    esp_err_t err = ReadRequestBody(req, &body);
    if (err != ESP_OK) {
        return SendError(req, "400 Bad Request", "Invalid form body");
    }
    const FormFields form = ParseForm(body);
    const std::string action = FormValue(form, "action");
    DeviceSettings candidate = *portal->settings_;

    if (action == "remove") {
        const size_t index = static_cast<size_t>(std::strtoul(FormValue(form, "index", "999").c_str(), nullptr, 10));
        if (index >= candidate.wifi_count) {
            return SendError(req, "400 Bad Request", "Saved Wi-Fi slot not found");
        }
        for (size_t i = index; i + 1 < candidate.wifi_count; ++i) {
            candidate.wifi[i] = candidate.wifi[i + 1];
        }
        candidate.wifi[candidate.wifi_count - 1] = WifiCredential{};
        candidate.wifi_count--;
    } else {
        const std::string ssid = FormValue(form, "ssid");
        const std::string password = FormValue(form, "password");
        if (ssid.empty()) {
            return SendError(req, "400 Bad Request", "SSID is required");
        }
        bool updated = false;
        for (size_t i = 0; i < candidate.wifi_count; ++i) {
            if (candidate.wifi[i].ssid == ssid) {
                candidate.wifi[i].password = password;
                updated = true;
                break;
            }
        }
        if (!updated) {
            if (candidate.wifi_count >= kMaxWifiCredentials) {
                return SendError(req, "400 Bad Request", "Wi-Fi list is full");
            }
            candidate.wifi[candidate.wifi_count++] = WifiCredential{ssid, password};
        }
        // No live STA test here. On a single-radio ESP32-C6, associating to the
        // home network in AP+STA mode yanks the SoftAP onto the home channel and
        // drops the phone's portal session mid-test (the "it just rebooted, no
        // feedback" symptom). Instead we save the network and let the user hit
        // Restart; the device connects on a clean STA boot and shows the result
        // on its own screen ("Connesso a X" or a "couldn't reach X" notice).
    }

    *portal->settings_ = candidate;
    err = portal->SaveCurrentSettings();
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    portal->reboot_required_ = true;  // the Wi-Fi list changed; restart to apply
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleSettings(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr || portal->settings_ == nullptr) {
        return SendError(req, "500 Internal Server Error", "Configuration is not ready");
    }
    std::string body;
    esp_err_t err = ReadRequestBody(req, &body);
    if (err != ESP_OK) {
        return SendError(req, "400 Bad Request", "Invalid form body");
    }
    const std::string language = FormValue(ParseForm(body), "language", "en");
    if (language != "en" && language != "es" && language != "it") {
        return SendError(req, "400 Bad Request", "Unsupported language");
    }
    portal->settings_->language = language;
    err = portal->SaveCurrentSettings();
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    // Language is applied cleanly by re-rendering on a fresh boot, so it follows
    // the same "apply = restart" rule as Wi-Fi: raise the sticky restart bar.
    portal->reboot_required_ = true;
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleOpenAi(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr || portal->settings_ == nullptr) {
        return SendError(req, "500 Internal Server Error", "Configuration is not ready");
    }
    std::string body;
    esp_err_t err = ReadRequestBody(req, &body);
    if (err != ESP_OK) {
        return SendError(req, "400 Bad Request", "Invalid form body");
    }
    const FormFields form = ParseForm(body);
    const std::string api_key = FormValue(form, "api_key");
    if (!api_key.empty()) {
        portal->settings_->openai.api_key = api_key;
    }
    const std::string transcription_model = FormValue(form, "transcription_model");
    const std::string response_model = FormValue(form, "response_model");
    const std::string speech_model = FormValue(form, "speech_model");
    const std::string voice = FormValue(form, "voice");
    if (!transcription_model.empty()) {
        portal->settings_->openai.transcription_model = transcription_model;
    }
    if (!response_model.empty()) {
        portal->settings_->openai.response_model = response_model;
    }
    if (!speech_model.empty()) {
        portal->settings_->openai.speech_model = speech_model;
    }
    if (!voice.empty()) {
        portal->settings_->openai.voice = voice;
    }
    const std::string reasoning = FormValue(form, "reasoning_effort");
    if (reasoning == "off") {
        portal->settings_->openai.reasoning_effort.clear();  // explicit off (non-reasoning models)
    } else if (!reasoning.empty()) {
        portal->settings_->openai.reasoning_effort = reasoning;
    }
    err = portal->SaveCurrentSettings();
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleEmail(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr || portal->settings_ == nullptr) {
        return SendError(req, "500 Internal Server Error", "Configuration is not ready");
    }
    std::string body;
    esp_err_t err = ReadRequestBody(req, &body);
    if (err != ESP_OK) {
        return SendError(req, "400 Bad Request", "Invalid form body");
    }
    const FormFields form = ParseForm(body);
    portal->settings_->email.enabled = FormValue(form, "enabled", "0") == "1";
    portal->settings_->email.recipient = FormValue(form, "recipient");
    const std::string relay_url = FormValue(form, "relay_url");
    if (!relay_url.empty()) {
        portal->settings_->email.relay_url = relay_url;
    }
    const std::string relay_token = FormValue(form, "relay_token");
    if (!relay_token.empty()) {
        portal->settings_->email.relay_token = relay_token;
    }
    err = portal->SaveCurrentSettings();
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleReset(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr || portal->config_store_ == nullptr || portal->settings_ == nullptr) {
        return SendError(req, "500 Internal Server Error", "Configuration is not ready");
    }
    esp_err_t err = portal->config_store_->Reset();
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    ApplyDefaultSettings(portal->settings_);
    portal->reboot_required_ = false;
    return portal->SendStatusJson(req);
}

esp_err_t WebPortal::HandleReboot(httpd_req_t *req) {
    WebPortal *portal = PortalFromRequest(req);
    if (portal == nullptr) {
        return SendError(req, "500 Internal Server Error", "Portal is not ready");
    }
    esp_err_t err = SendJson(req, "{\"rebooting\":true}");
    xTaskCreate(&RestartTask, "bisc8_reboot", 2048, nullptr, 5, nullptr);
    return err;
}

esp_err_t WebPortal::HandleCaptiveRedirect(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, nullptr, 0);
}

esp_err_t WebPortal::SendStatusJson(httpd_req_t *req) const {
    DebugSerial::Log("[WEB]", "GET /api/status free_heap=%u", static_cast<unsigned>(esp_get_free_heap_size()));
    std::string setup_ssid;
    std::string setup_url = "http://192.168.4.1";
    std::string connected_ssid;
    std::string connected_ip;
    std::string ssid_attempt;
    bool online = false;
    bool setup_active = running_;

    if (wifi_ != nullptr) {
        setup_ssid = wifi_->setup_ssid;
        setup_url = wifi_->setup_url.empty() ? setup_url : wifi_->setup_url;
        connected_ssid = wifi_->connected_ssid;
        connected_ip = wifi_->connected_ip;
        ssid_attempt = wifi_->ssid_attempt;
        online = wifi_->online;
        setup_active = wifi_->setup_active || running_;
    }

    std::string language = "en";
    std::string openai_key;
    std::string email_recipient;
    std::string email_relay;
    bool email_enabled = false;
    size_t wifi_count = 0;
    if (settings_ != nullptr) {
        language = settings_->language;
        openai_key = settings_->openai.api_key.empty() ? "missing" : MaskSecret(settings_->openai.api_key);
        email_recipient = settings_->email.recipient.empty() ? "missing" : MaskSecret(settings_->email.recipient);
        email_relay = settings_->email.relay_url.empty() ? "missing" : "configured";
        email_enabled = settings_->email.enabled;
        wifi_count = settings_->wifi_count;
    }

    const std::string wifi_mode = online ? "online" : (setup_active ? "setup mode" : "offline");
    const std::string device_address = online && !connected_ip.empty() ? connected_ip : setup_url;
    const std::string network_label = online ? connected_ssid : wifi_mode;

    std::string json = "{";
    json += "\"online\":" + std::string(online ? "true" : "false");
    json += ",\"setup_active\":" + std::string(setup_active ? "true" : "false");
    json += ",\"setup_ssid\":" + JsonString(setup_ssid);
    json += ",\"setup_url\":" + JsonString(setup_url);
    json += ",\"connected_ip\":" + JsonString(connected_ip.empty() ? "missing" : connected_ip);
    json += ",\"device_address\":" + JsonString(device_address);
    json += ",\"wifi_mode\":" + JsonString(wifi_mode);
    json += ",\"network_label\":" + JsonString(network_label.empty() ? wifi_mode : network_label);
    json += ",\"connected_ssid\":" + JsonString(connected_ssid.empty() ? "setup mode" : connected_ssid);
    json += ",\"ssid_attempt\":" + JsonString(ssid_attempt);
    json += ",\"language\":" + JsonString(language);
    json += ",\"wifi_count\":" + std::to_string(wifi_count);
    json += ",\"reboot_required\":" + std::string(reboot_required_ ? "true" : "false");
    json += ",\"openai_key\":" + JsonString(openai_key);
    json += ",\"email_enabled\":" + std::string(email_enabled ? "true" : "false");
    json += ",\"email_recipient\":" + JsonString(email_recipient);
    json += ",\"email_relay\":" + JsonString(email_relay);
    json += ",\"build_version\":" + JsonString(BISC8_BUILD_VERSION);
    json += ",\"warning\":\"secrets are stored on this device\"";
    json += "}";
    return SendJson(req, json);
}

esp_err_t WebPortal::SendWifiScanJson(httpd_req_t *req) const {
    DebugSerial::LogAlways("[WEB]", "GET /api/wifi/scan start free_heap=%u", static_cast<unsigned>(esp_get_free_heap_size()));
    wifi_scan_config_t scan_config = {};
    scan_config.show_hidden = false;
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }
    ap_count = std::min(ap_count, kMaxScanRecords);
    wifi_ap_record_t records[kMaxScanRecords] = {};
    err = esp_wifi_scan_get_ap_records(&ap_count, records);
    if (err != ESP_OK) {
        return SendError(req, "500 Internal Server Error", esp_err_to_name(err));
    }

    std::string json = "{\"networks\":[";
    for (uint16_t i = 0; i < ap_count; ++i) {
        if (i > 0) {
            json += ",";
        }
        json += "{\"ssid\":";
        json += JsonString(reinterpret_cast<const char *>(records[i].ssid));
        json += ",\"rssi\":";
        json += std::to_string(records[i].rssi);
        json += ",\"channel\":";
        json += std::to_string(records[i].primary);
        json += ",\"band\":";
        json += JsonString(WifiBandLabel(records[i].primary));
        json += "}";
    }
    json += "]}";
    DebugSerial::LogAlways("[WEB]",
                           "GET /api/wifi/scan done count=%u free_heap=%u",
                           static_cast<unsigned>(ap_count),
                           static_cast<unsigned>(esp_get_free_heap_size()));
    return SendJson(req, json);
}

esp_err_t WebPortal::SaveCurrentSettings() {
    if (config_store_ == nullptr || settings_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    return config_store_->Save(*settings_);
}

esp_err_t WebPortal::Start() {
    if (running_) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.lru_purge_enable = true;
    config.max_open_sockets = 3;
    config.backlog_conn = 2;
    // The "Save Wi-Fi" handler tests the credentials synchronously, which now
    // blocks up to kWifiAttemptTimeoutMs (10s) while ConnectToNetwork re-kicks
    // the association. Keep the socket timeouts comfortably above that so the
    // success/failure response actually reaches the browser instead of the
    // socket being closed mid-test (which looked like "no feedback / reboot").
    config.recv_wait_timeout = 16;
    config.send_wait_timeout = 16;
    config.max_uri_handlers = sizeof(kPortalRoutes) / sizeof(kPortalRoutes[0]) +
                              sizeof(kCaptiveProbePaths) / sizeof(kCaptiveProbePaths[0]);

    esp_err_t err = httpd_start(&server_, &config);
    if (err != ESP_OK) {
        DebugSerial::LogAlways("[WEB]", "setup portal failed to start: %s", esp_err_to_name(err));
        return err;
    }

    err = RegisterRoute("/", HTTP_GET, &WebPortal::HandleIndex);
    if (err == ESP_OK) {
        err = RegisterRoute("/api/status", HTTP_GET, &WebPortal::HandleStatus);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/wifi/scan", HTTP_GET, &WebPortal::HandleWifiScan);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/wifi/credentials", HTTP_POST, &WebPortal::HandleWifiCredentials);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/settings", HTTP_POST, &WebPortal::HandleSettings);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/openai", HTTP_POST, &WebPortal::HandleOpenAi);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/email", HTTP_POST, &WebPortal::HandleEmail);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/smtp", HTTP_POST, &WebPortal::HandleEmail);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/reset", HTTP_POST, &WebPortal::HandleReset);
    }
    if (err == ESP_OK) {
        err = RegisterRoute("/api/reboot", HTTP_POST, &WebPortal::HandleReboot);
    }
    for (const char *probe : kCaptiveProbePaths) {
        if (err != ESP_OK) {
            break;
        }
        err = RegisterRoute(probe, HTTP_GET, &WebPortal::HandleCaptiveRedirect);
    }

    if (err != ESP_OK) {
        Stop();
        return err;
    }

    DebugSerial::LogAlways("[WEB]", "setup portal ready at http://192.168.4.1 with masked secrets: %s",
                           MaskSecret("sk-example").c_str());
    running_ = true;
    return ESP_OK;
}

void WebPortal::Stop() {
    if (server_ != nullptr) {
        httpd_stop(server_);
        server_ = nullptr;
    }
    if (running_) {
        DebugSerial::LogAlways("[WEB]", "setup portal stopped");
    }
    running_ = false;
}

bool WebPortal::Running() const {
    return running_;
}

bool WebPortal::ConsumeActivity() {
    const bool had = activity_;
    activity_ = false;
    return had;
}

}  // namespace bisc8
