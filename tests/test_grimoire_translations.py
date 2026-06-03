from pathlib import Path


SOURCE = Path("assets/grimorio.txt")
TRANSLATIONS = [
    Path("assets/grimorio.en.txt"),
    Path("assets/grimorio.es.txt"),
]
MAX_LINE_LENGTH = 120


def _lines(path: Path) -> list[str]:
    return path.read_text(encoding="utf-8").splitlines()


def test_grimoire_translation_files_match_source_count_and_limits():
    source_lines = _lines(SOURCE)

    assert len(source_lines) == 888

    for translation in TRANSLATIONS:
        translated_lines = _lines(translation)

        assert len(translated_lines) == len(source_lines)
        assert all(line.strip() for line in translated_lines)
        assert max(len(line) for line in translated_lines) <= MAX_LINE_LENGTH
