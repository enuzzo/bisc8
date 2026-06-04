#!/usr/bin/env sh
# Source this file before running ESP-IDF commands for Bisc8.

BISC8_ENV_FILE="$0"
if [ -n "${BASH_SOURCE:-}" ]; then
    BISC8_ENV_FILE="${BASH_SOURCE}"
elif [ -n "${ZSH_VERSION:-}" ]; then
    BISC8_ENV_FILE="${(%):-%x}"
fi

BISC8_SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "${BISC8_ENV_FILE}")" && pwd)
BISC8_ROOT=$(CDPATH= cd -- "${BISC8_SCRIPT_DIR}/.." && pwd)
BISC8_HOST_ARCH=$(uname -m)

case "${BISC8_HOST_ARCH}" in
    arm64|aarch64)
        BISC8_IDF_ARCH="arm64"
        ;;
    x86_64|amd64)
        BISC8_IDF_ARCH="x86_64"
        ;;
    *)
        BISC8_IDF_ARCH="${BISC8_HOST_ARCH}"
        ;;
esac

export IDF_PATH="${BISC8_IDF_PATH:-${BISC8_ROOT}/.esp/esp-idf}"
export IDF_TOOLS_PATH="${BISC8_IDF_TOOLS_PATH:-${HOME}/.espressif-bisc8-${BISC8_IDF_ARCH}}"
export IDF_PYTHON_ENV_PATH="${BISC8_IDF_PYTHON_ENV_PATH:-${IDF_TOOLS_PATH}/python_env/idf5.5_py3.9_env}"

if [ -d "${IDF_PYTHON_ENV_PATH}/bin" ]; then
    PATH="${IDF_PYTHON_ENV_PATH}/bin:${PATH}"
fi
if [ -d "${IDF_TOOLS_PATH}/tools/riscv32-esp-elf/esp-14.2.0_20251107/riscv32-esp-elf/bin" ]; then
    PATH="${IDF_TOOLS_PATH}/tools/riscv32-esp-elf/esp-14.2.0_20251107/riscv32-esp-elf/bin:${PATH}"
fi
if [ -d "${IDF_TOOLS_PATH}/tools/riscv32-esp-elf-gdb/16.3_20250913/riscv32-esp-elf-gdb/bin" ]; then
    PATH="${IDF_TOOLS_PATH}/tools/riscv32-esp-elf-gdb/16.3_20250913/riscv32-esp-elf-gdb/bin:${PATH}"
fi
if [ -d "${IDF_TOOLS_PATH}/tools/openocd-esp32/v0.12.0-esp32-20251215/openocd-esp32/bin" ]; then
    PATH="${IDF_TOOLS_PATH}/tools/openocd-esp32/v0.12.0-esp32-20251215/openocd-esp32/bin:${PATH}"
fi
if [ -d "${IDF_PATH}/tools" ]; then
    PATH="${IDF_PATH}/tools:${PATH}"
fi
export PATH

echo "Bisc8 ESP-IDF environment:"
echo "  IDF_PATH=${IDF_PATH}"
echo "  IDF_TOOLS_PATH=${IDF_TOOLS_PATH}"
echo "  host=${BISC8_HOST_ARCH} tools=${BISC8_IDF_ARCH}"
