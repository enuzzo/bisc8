from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/bisc8_fortune/main"
PORT = ROOT / "firmware/bisc8_fortune/components/port_bsp"

DISPLAY_CPP = MAIN / "display_service.cpp"
DISPLAY_H = MAIN / "display_service.h"
AUDIO_CPP = MAIN / "audio_service.cpp"
AUDIO_H = MAIN / "audio_service.h"
APP_MAIN = MAIN / "app_main.cpp"
APP_EVENTS = MAIN / "app_events.h"
LOCALIZATION = MAIN / "localization.cpp"
PORT_DISPLAY_CPP = PORT / "port_display.cpp"
PORT_DISPLAY_H = PORT / "port_display.h"

# Long dashes are banned everywhere in the Netmilk copy.
EM_DASH = "—"
EN_DASH = "–"


def test_dedicated_state_screens_are_exposed():
    header = DISPLAY_H.read_text(encoding="utf-8")
    source = DISPLAY_CPP.read_text(encoding="utf-8")

    for method in ("ShowNoWifi", "ShowLowBattery", "ShowFirstRun"):
        assert f"void {method}(Language language);" in header
        assert f"DisplayService::{method}" in source

    # No-wifi and low-battery share the centered-glyph layout; first-run reuses
    # the message layout.
    assert "LayoutGlyphMessage" in source
    assert "strings.no_wifi_body" in source
    assert "strings.low_battery_body" in source
    assert "strings.first_run_body" in source


def test_speaker_glyph_animation_is_driven_by_audio_hooks():
    header = DISPLAY_H.read_text(encoding="utf-8")
    source = DISPLAY_CPP.read_text(encoding="utf-8")

    # Animated speaker glyph built from blocks with toggled sound waves.
    assert "BuildSpeaker" in source
    assert "speaker_wave1_" in source and "speaker_wave3_" in source
    assert "lv_timer_create" in source
    assert "StartSpeakingAnimation" in source
    assert "StopSpeakingAnimation" in source

    # The speaking screen reveals the answer and starts the animation.
    speaking = source.split("DisplayService::ShowVoiceSpeaking", 1)[1].split("DisplayService::", 1)[0]
    assert "LayoutSpeaking()" in speaking
    assert "StartSpeakingAnimation()" in speaking

    # Display exposes the audio-driven hooks; the listening hook is the future
    # "ti ascolto" seam.
    assert "void OnPlaybackState(bool active);" in header
    assert "void OnListeningState(bool active);" in header


def test_audio_service_emits_playback_and_recording_hooks():
    header = AUDIO_H.read_text(encoding="utf-8")
    source = AUDIO_CPP.read_text(encoding="utf-8")

    assert "void SetPlaybackObserver(AudioStateHook hook, void *ctx);" in header
    assert "void SetRecordingObserver(AudioStateHook hook, void *ctx);" in header
    assert "NotifyPlayback(true)" in source
    assert "NotifyPlayback(false)" in source
    assert "NotifyRecording(true)" in source
    assert "NotifyRecording(false)" in source


def test_app_main_wires_hooks_and_battery_first_run_triggers():
    source = APP_MAIN.read_text(encoding="utf-8")
    events = APP_EVENTS.read_text(encoding="utf-8")

    assert "audio.SetPlaybackObserver(&OnAudioPlayback, &display);" in source
    assert "audio.SetRecordingObserver(&OnAudioRecording, &display);" in source
    assert "battery_is_low()" in source
    assert "display.ShowLowBattery(startup_language);" in source
    assert "display.ShowFirstRun(startup_language);" in source
    assert "g_fortune_count == 0" in source

    # Serial previews so every screen is reachable for snapshot validation.
    assert 'strncmp(line, "SCREEN ", 7)' in source
    for event in ("PreviewNoWifi", "PreviewLowBattery", "PreviewFirstRun", "PreviewSpeaking"):
        assert event in events
        assert event in source


def test_eink_refresh_policy_lives_at_the_port_boundary():
    port_h = PORT_DISPLAY_H.read_text(encoding="utf-8")
    port_c = PORT_DISPLAY_CPP.read_text(encoding="utf-8")
    display = DISPLAY_CPP.read_text(encoding="utf-8")

    # Full-refresh primitive added to the port layer.
    assert "void EPD_DisplayFull();" in port_h
    assert "void EPD_DisplayFull()" in port_c

    # Policy: full refresh on reveals, forced full after N partials.
    assert "kMaxPartialsBeforeFull" in display
    assert "RequestFullRefresh" in display
    assert "EPD_DisplayFull();" in display
    assert "EPD_DisplayPart();" in display

    # The responso reveal is a full refresh.
    fortune = display.split("DisplayService::ShowFortune", 1)[1].split("DisplayService::", 1)[0]
    assert "RequestFullRefresh()" in fortune


def test_footer_battery_icon_tracks_charge():
    source = DISPLAY_CPP.read_text(encoding="utf-8")

    assert "BuildFooterBattery" in source
    assert "batt_icon_fill_" in source
    # Fill width is derived from the charge percentage.
    assert "lv_obj_set_width(batt_icon_fill_" in source


def test_new_state_copy_is_localized_in_netmilk_voice():
    source = LOCALIZATION.read_text(encoding="utf-8")

    # Italian anchors from the brief.
    assert "Niente Wi-Fi." in source
    assert "Quasi spenta." in source
    assert "Zero responsi caricati." in source
    # Transcreated, not left in one language.
    assert "Sin Wi-Fi." in source
    assert "No Wi-Fi." in source
    assert "Casi muerta." in source


def test_no_long_dashes_anywhere_in_localized_copy():
    source = LOCALIZATION.read_text(encoding="utf-8")
    assert EM_DASH not in source
    assert EN_DASH not in source
