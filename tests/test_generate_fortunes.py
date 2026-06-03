from pathlib import Path

import pytest

from tools.generate_fortunes import DEFAULT_SOURCE, DEFAULT_SOURCES_BY_LANGUAGE, generate_header, parse_fortunes


def test_default_source_is_grimorio():
    assert DEFAULT_SOURCE == Path("assets/grimorio.txt")
    assert DEFAULT_SOURCES_BY_LANGUAGE["it"] == Path("assets/grimorio.txt")
    assert DEFAULT_SOURCES_BY_LANGUAGE["en"] == Path("assets/grimorio.en.txt")
    assert DEFAULT_SOURCES_BY_LANGUAGE["es"] == Path("assets/grimorio.es.txt")


def test_parse_fortunes_ignores_blank_lines_and_preserves_order(tmp_path: Path):
    source = tmp_path / "fortunes.txt"
    source.write_text("\nLa luna dice si.\n\nRiprova al crepuscolo.\n", encoding="utf-8")

    fortunes = parse_fortunes(source)

    assert fortunes == ["La luna dice si.", "Riprova al crepuscolo."]


def test_generate_header_escapes_strings_and_exposes_count():
    header = generate_header(['La "soglia" risponde.', "Via\\ombra"], language="it")

    assert "kFortuneCount = 2" in header
    assert 'kFortuneLanguage[] = "it"' in header
    assert '"La \\"soglia\\" risponde."' in header
    assert '"Via\\\\ombra"' in header


def test_generate_header_supports_language_specific_symbol_names():
    header = generate_header(["The moon says yes."], language="en")

    assert "kFortunes_en" in header
    assert "kFortuneCount_en = 1" in header
    assert 'kFortuneLanguage_en[] = "en"' in header


def test_parse_fortunes_rejects_lines_over_hard_limit(tmp_path: Path):
    source = tmp_path / "fortunes.txt"
    source.write_text("x" * 121, encoding="utf-8")

    with pytest.raises(ValueError, match="line 1"):
        parse_fortunes(source)
