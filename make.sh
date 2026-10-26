#!/usr/bin/env bash
# Build helper for SmartCardManager on Linux (Qt 6 + qmake + pcsc-lite).
#
#   ./make.sh [command] [options]
#
# Commands:
#   build    configure and compile the application (default)
#   test     build and run the unit tests (headless, QT_QPA_PLATFORM=offscreen)
#   run      build, then start the application
#   clean    remove the build output (build/release, build/debug, build/tests)
#   deps     only check that the required tools/libraries are installed
#   help     show this text
#
# Options:
#   -d, --debug       debug build (build/debug) instead of release (build/release)
#   -j, --jobs N      parallel jobs (default: number of CPUs)
#   -v, --verbose     show full compiler command lines
#   -- ARGS...        (test) arguments passed to the test runner, e.g.  ./make.sh test -- TstHexEdit
#
# Environment:
#   QMAKE             path of the qmake binary (default: qmake6, then qmake)
#
# Required packages (Debian/Ubuntu | Arch | Fedora):
#   qt6-base-dev qt6-svg-dev libpcsclite-dev pkg-config g++ make
#   qt6-base qt6-svg pcsclite pkgconf gcc make
#   qt6-qtbase-devel qt6-qtsvg-devel pcsc-lite-devel pkgconf-pkg-config gcc-c++ make
# Runtime: the pcscd service must be running to talk to readers.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_NAME="release"
JOBS="$(nproc 2>/dev/null || echo 2)"
VERBOSE=0
COMMAND="build"
EXTRA_ARGS=()

die()  { echo "error: $*" >&2; exit 1; }
info() { echo "==> $*"; }

usage() { sed -n '2,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

find_qmake() {
    if [[ -n "${QMAKE:-}" ]]; then echo "$QMAKE"; return; fi
    for candidate in qmake6 qmake-qt6 qmake; do
        if command -v "$candidate" >/dev/null 2>&1; then
            # must be a Qt 6 qmake
            if "$candidate" -query QT_VERSION 2>/dev/null | grep -q '^6\.'; then
                echo "$candidate"; return
            fi
        fi
    done
}

check_deps() {
    local missing=0
    QMAKE_BIN="$(find_qmake || true)"
    if [[ -z "$QMAKE_BIN" ]]; then echo "missing: Qt 6 qmake (qmake6)" >&2; missing=1; fi
    command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 || { echo "missing: C++ compiler (g++/clang++)" >&2; missing=1; }
    command -v make >/dev/null 2>&1 || { echo "missing: make" >&2; missing=1; }
    if command -v pkg-config >/dev/null 2>&1; then
        pkg-config --exists libpcsclite || { echo "missing: libpcsclite development files (pcsc-lite)" >&2; missing=1; }
    elif [[ ! -f /usr/include/PCSC/winscard.h ]]; then
        echo "missing: pkg-config or /usr/include/PCSC/winscard.h (pcsc-lite headers)" >&2; missing=1
    fi
    if [[ -n "$QMAKE_BIN" ]]; then
        local qtlib; qtlib="$("$QMAKE_BIN" -query QT_INSTALL_LIBS)"
        compgen -G "$qtlib/libQt6SvgWidgets.so*" >/dev/null || \
            compgen -G "/usr/lib*/*/libQt6SvgWidgets.so*" >/dev/null || \
            echo "warning: Qt6 SVG module not found (qt6-svg), build will probably fail" >&2
    fi
    [[ $missing -eq 0 ]] || die "install the missing dependencies (see header of this script)"
    info "using $("$QMAKE_BIN" -query QT_VERSION | sed 's/^/Qt /') via $QMAKE_BIN, $JOBS jobs"
}

configure_and_make() {   # <source dir> <pro file> <build dir> <qmake config>
    local src="$1" pro="$2" out="$3" cfg="$4"
    mkdir -p "$out"
    ( cd "$out"
      "$QMAKE_BIN" "$src/$pro" "CONFIG+=$cfg"
      if [[ $VERBOSE -eq 1 ]]; then make -j"$JOBS" VERBOSE=1 V=1; else make -j"$JOBS"; fi )
}

do_build() {
    check_deps
    info "building application ($CONFIG_NAME)"
    configure_and_make "$ROOT" "SmartCardManager.pro" "$ROOT/build/$CONFIG_NAME" "$CONFIG_NAME"
    info "done: build/$CONFIG_NAME/SmartCardManager"
}

do_test() {
    check_deps
    info "building tests (debug, asserts enabled)"
    configure_and_make "$ROOT/tests" "tests.pro" "$ROOT/build/tests" "debug"
    info "running tests"
    cd "$ROOT/build/tests"
    QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}" ./tst_smartcardmanager "${EXTRA_ARGS[@]}"
}

do_run() {
    do_build
    exec "$ROOT/build/$CONFIG_NAME/SmartCardManager"
}

do_clean() {
    info "removing build/release build/debug build/tests"
    rm -rf "$ROOT/build/release" "$ROOT/build/debug" "$ROOT/build/tests"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        build|test|run|clean|deps|help) COMMAND="$1" ;;
        -d|--debug)   CONFIG_NAME="debug" ;;
        -j|--jobs)    shift; [[ $# -gt 0 && "$1" =~ ^[0-9]+$ ]] || die "--jobs needs a number"; JOBS="$1" ;;
        -v|--verbose) VERBOSE=1 ;;
        -h|--help)    COMMAND="help" ;;
        --)           shift; EXTRA_ARGS=("$@"); break ;;
        *)            die "unknown argument '$1' (try ./make.sh help)" ;;
    esac
    shift
done

case "$COMMAND" in
    build) do_build ;;
    test)  do_test ;;
    run)   do_run ;;
    clean) do_clean ;;
    deps)  check_deps; info "all dependencies found" ;;
    help)  usage ;;
esac
