#!/usr/bin/env bash
# Sets up the Fast-DDS environment and generates type-support code from IDL files.
#
# Usage:
#   ./scripts/setup_fastdds.sh              # Set env + generate type support
#   ./scripts/setup_fastdds.sh --clean      # Remove generated/ first, then generate
#   ./scripts/setup_fastdds.sh --env-only   # Only export environment (for sourcing)
#
# Source this script if you want the env vars to persist in your current shell:
#   source ./scripts/setup_fastdds.sh --env-only
#
# Override defaults with environment variables:
#   FASTDDS_INSTALL=/custom/path ./scripts/setup_fastdds.sh
#   FASTDDSGEN_BIN=/custom/path/fastddsgen ./scripts/setup_fastdds.sh

set -euo pipefail

# Fast-DDS environment
FASTDDS_INSTALL="${FASTDDS_INSTALL:-$HOME/Fast-DDS/install}"

if [[ ! -d "${FASTDDS_INSTALL}" ]]; then
    echo "ERROR: Fast-DDS install directory not found: ${FASTDDS_INSTALL}" >&2
    echo "       Set FASTDDS_INSTALL=/your/path or install Fast-DDS to ~/Fast-DDS/install" >&2
    exit 1
fi

export LD_LIBRARY_PATH="${FASTDDS_INSTALL}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export PATH="${FASTDDS_INSTALL}/bin${PATH:+:${PATH}}"
export CMAKE_PREFIX_PATH="${FASTDDS_INSTALL}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"

echo "Fast-DDS environment loaded from: ${FASTDDS_INSTALL}"
echo "  PATH              += ${FASTDDS_INSTALL}/bin"
echo "  LD_LIBRARY_PATH   += ${FASTDDS_INSTALL}/lib"
echo "  CMAKE_PREFIX_PATH += ${FASTDDS_INSTALL}"
echo

if [[ "${1:-}" == "--env-only" ]]; then
    return 0 2>/dev/null || exit 0
fi

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

IDL_DIR="${REPO_ROOT}/idl"
GEN_DIR="${REPO_ROOT}/generated"

if [[ "${1:-}" == "--clean" ]]; then
    echo "Removing ${GEN_DIR}..."
    rm -rf "${GEN_DIR}"
    echo
fi

# Locate FastDDSGen
if [[ -n "${FASTDDSGEN_BIN:-}" ]]; then
    FASTDDSGEN="${FASTDDSGEN_BIN}"
elif command -v fastddsgen &>/dev/null; then
    FASTDDSGEN="$(command -v fastddsgen)"
elif [[ -x "$HOME/Fast-DDS/src/fastddsgen/scripts/fastddsgen" ]]; then
    FASTDDSGEN="$HOME/Fast-DDS/src/fastddsgen/scripts/fastddsgen"
elif [[ -x "${FASTDDS_INSTALL}/bin/fastddsgen" ]]; then
    FASTDDSGEN="${FASTDDS_INSTALL}/bin/fastddsgen"
else
    echo "ERROR: fastddsgen not found. Tried:" >&2
    echo "  1. \$FASTDDSGEN_BIN env var (not set)" >&2
    echo "  2. PATH" >&2
    echo "  3. $HOME/Fast-DDS/src/fastddsgen/scripts/fastddsgen" >&2
    echo "  4. ${FASTDDS_INSTALL}/bin/fastddsgen" >&2
    echo >&2
    echo "  Set FASTDDSGEN_BIN=/path/to/fastddsgen and retry." >&2
    exit 1
fi

FASTDDSGEN_VERSION="$("${FASTDDSGEN}" -version 2>&1 | grep -i "Fast" | head -1 || echo "unknown")"
echo "Using fastddsgen: ${FASTDDSGEN}"
echo "  Version : ${FASTDDSGEN_VERSION}"
echo "  Source  : ${IDL_DIR}"
echo "  Output  : ${GEN_DIR}"
echo

# Check there are IDL files to process
mapfile -t IDL_FILES < <(find "${IDL_DIR}" -name '*.idl' | sort)

if [[ ${#IDL_FILES[@]} -eq 0 ]]; then
    echo "WARNING: No .idl files found under ${IDL_DIR}" >&2
    exit 0
fi

# Generate type support for each IDL file,
for idl_file in "${IDL_FILES[@]}"; do
    out_dir="${GEN_DIR}"

    mkdir -p "${out_dir}"

    echo "  [GEN] ${idl_file#"${REPO_ROOT}/"}"
    echo "      → ${out_dir#"${REPO_ROOT}/"}"
    "${FASTDDSGEN}" -typeros2 -no-typeobjectsupport -no-dependencies -replace -I "${IDL_DIR}" -d "${GEN_DIR}" "${idl_file}" 2>&1 \
        | grep -v "^openjdk\|^OpenJDK\|64-Bit Server"
    [[ ${PIPESTATUS[0]} -eq 0 ]] || { echo "ERROR: fastddsgen failed for ${idl_file}" >&2; exit 1; }
done

echo
echo "Done. ${#IDL_FILES[@]} IDL file(s) processed."