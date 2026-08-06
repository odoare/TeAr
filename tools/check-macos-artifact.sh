#!/usr/bin/env bash
# Verify a macOS release artifact from Linux, without a Mac.
#
# Usage:  tools/check-macos-artifact.sh TeAr-VST3-macOS-universal.zip [more.zip ...]
#         tools/check-macos-artifact.sh path/to/TeAr.vst3
#
# Checks every Mach-O executable found inside each argument for both the
# x86_64 and arm64 slices, and reports the minimum macOS version each slice
# was built against. Exits non-zero if anything is missing.

set -uo pipefail

WANT_ARCHS=(x86_64 arm64)
MAX_MINOS=10.15   # informational: a slice with minos above this excludes Catalina

# llvm-lipo ships in the llvm-N package but is usually not on PATH.
LIPO=$(command -v llvm-lipo || true)
if [[ -z "$LIPO" ]]; then
    LIPO=$(ls -1 /usr/lib/llvm-*/bin/llvm-lipo 2>/dev/null | sort -V | tail -1 || true)
fi
if [[ -z "$LIPO" ]]; then
    echo "error: llvm-lipo not found. Install it with:  sudo apt install llvm" >&2
    exit 2
fi

OTOOL=$(command -v llvm-otool || true)
[[ -z "$OTOOL" ]] && OTOOL=$(ls -1 /usr/lib/llvm-*/bin/llvm-otool 2>/dev/null | sort -V | tail -1 || true)

status=0
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

check_binary() {
    local bin=$1 label=$2

    local archs
    if ! archs=$("$LIPO" -archs "$bin" 2>/dev/null); then
        echo "  !! $label: not a Mach-O binary"
        status=1
        return
    fi

    local missing=()
    for want in "${WANT_ARCHS[@]}"; do
        grep -qw "$want" <<< "$archs" || missing+=("$want")
    done

    if (( ${#missing[@]} )); then
        echo "  FAIL $label"
        echo "       has: $archs"
        echo "       missing: ${missing[*]}"
        status=1
    else
        echo "  ok   $label  [$archs]"
    fi

    # Minimum macOS version per slice (the vtool -show-build equivalent).
    # llvm-otool only reports the FIRST slice of a fat file, so thin each one.
    if [[ -n "$OTOOL" ]]; then
        local arch slice minos
        for arch in $archs; do
            slice="$tmpdir/slice.$$"
            "$LIPO" "$bin" -thin "$arch" -output "$slice" 2>/dev/null || continue
            minos=$("$OTOOL" -l "$slice" 2>/dev/null \
                    | grep -E '^\s*(minos|version)\s' \
                    | awk '{print $2}' | head -1)
            rm -f "$slice"
            [[ -z "$minos" ]] && continue
            if [[ "$arch" == x86_64 ]] \
               && [[ "$(printf '%s\n%s\n' "$minos" "$MAX_MINOS" | sort -V | head -1)" != "$minos" ]]; then
                echo "       WARN $arch minos $minos > $MAX_MINOS (excludes older macOS)"
                status=1
            else
                echo "       $arch minos $minos"
            fi
        done
    fi
}

# Every Mach-O executable in a bundle lives under Contents/MacOS/.
scan_tree() {
    local root=$1 origin=$2
    local found=0
    while IFS= read -r -d '' bin; do
        found=1
        check_binary "$bin" "${bin#"$root"/}"
    done < <(find "$root" -type f -path '*/Contents/MacOS/*' -print0)

    if (( ! found )); then
        echo "  !! no Contents/MacOS/* executable found in $origin"
        status=1
    fi
}

for arg in "$@"; do
    echo "== $arg"
    if [[ ! -e "$arg" ]]; then
        echo "  !! no such file"
        status=1
    elif [[ -d "$arg" ]]; then
        scan_tree "$arg" "$arg"
    elif [[ "$arg" == *.zip ]]; then
        dest="$tmpdir/$(basename "$arg" .zip)"
        mkdir -p "$dest"
        if unzip -q "$arg" -d "$dest"; then
            scan_tree "$dest" "$arg"
        else
            echo "  !! could not unzip"
            status=1
        fi
    else
        check_binary "$arg" "$(basename "$arg")"
    fi
    echo
done

if (( status )); then
    echo "RESULT: FAILED — do not ship this artifact."
else
    echo "RESULT: all binaries are universal (${WANT_ARCHS[*]})."
fi
exit $status
