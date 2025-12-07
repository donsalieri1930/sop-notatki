#!/usr/bin/env bash
set -euo pipefail

# Nazwa Twojego programu
P4="./p4"

######################
# Funkcje pomocnicze #
######################

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

assert_eq() {
    local expected="$1"
    local actual="$2"
    local msg="$3"
    if [[ "$expected" != "$actual" ]]; then
        fail "$msg (expected='$expected', actual='$actual')"
    fi
}

assert_exists() {
    local path="$1"
    local msg="${2:-file/dir '$path' does not exist}"
    [[ -e "$path" ]] || fail "$msg"
}

assert_not_exists() {
    local path="$1"
    local msg="${2:-file/dir '$path' should not exist}"
    [[ ! -e "$path" ]] || fail "$msg"
}

assert_symlink_target() {
    local link="$1"
    local expected_target="$2"
    local msg="$3"

    [[ -L "$link" ]] || fail "$msg (not a symlink: $link)"
    local actual
    actual="$(readlink "$link")"
    assert_eq "$expected_target" "$actual" "$msg"
}

start_daemon() {
    local src="$1"
    local dest="$2"

    "$P4" "$src" "$dest" &
    P4_PID=$!
    # czas na pierwszą kopię
    sleep 1
}

stop_daemon() {
    if [[ -n "${P4_PID:-}" ]]; then
        kill "$P4_PID" 2>/dev/null || true
        wait "$P4_PID" 2>/dev/null || true
        unset P4_PID
    fi
}

######################
#        TESTY       #
######################

# TEST 1:
# Usunięcie pojedynczego pliku w katalogu (bez kasowania całego poddrzewa).
test_delete_single_file() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"

    mkdir -p "$src/dir"
    echo "A" > "$src/dir/a.txt"
    echo "B" > "$src/dir/b.txt"

    start_daemon "$src" "$dest"

    assert_exists "$dest/dir/a.txt" "a.txt should exist initially"
    assert_exists "$dest/dir/b.txt" "b.txt should exist initially"

    # usuwamy tylko jeden plik
    rm "$src/dir/a.txt"

    sleep 1

    assert_not_exists "$dest/dir/a.txt" "a.txt should be removed in dest"
    assert_exists "$dest/dir/b.txt" "b.txt should still exist in dest"
    assert_eq "B" "$(cat "$dest/dir/b.txt")" "b.txt content should stay unchanged"

    stop_daemon
    rm -rf "$root"
}

# TEST 2:
# Zmiana nazwy pojedynczego pliku (mv plik → inna_nazwa) w tym samym katalogu.
test_rename_single_file() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"

    mkdir -p "$src/dir"
    echo "content" > "$src/dir/file.txt"

    start_daemon "$src" "$dest"

    assert_exists "$dest/dir/file.txt" "file.txt should exist initially"
    assert_eq "content" "$(cat "$dest/dir/file.txt")" "initial content mismatch"

    mv "$src/dir/file.txt" "$src/dir/renamed.txt"

    sleep 1

    assert_not_exists "$dest/dir/file.txt" "old name should disappear in dest"
    assert_exists "$dest/dir/renamed.txt" "renamed.txt should appear in dest"
    assert_eq "content" "$(cat "$dest/dir/renamed.txt")" "renamed file content mismatch"

    stop_daemon
    rm -rf "$root"
}

# TEST 3:
# Usunięcie istniejącego symlinka.
test_remove_symlink() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"

    mkdir -p "$src/dir"
    echo "X" > "$src/dir/file.txt"
    ln -s "file.txt" "$src/dir/link_rel"

    start_daemon "$src" "$dest"

    assert_exists "$dest/dir/file.txt" "file.txt should exist initially"
    assert_symlink_target "$dest/dir/link_rel" "file.txt" \
        "relative link should exist initially"

    # usuwamy sam symlink
    rm "$src/dir/link_rel"

    sleep 1

    assert_not_exists "$dest/dir/link_rel" "link_rel should be removed in dest"
    assert_exists "$dest/dir/file.txt" "file.txt should remain in dest"

    stop_daemon
    rm -rf "$root"
}

# TEST 4:
# Zmiana nazwy symlinka (mv link1 → link2) w tym samym katalogu.
test_rename_symlink() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"

    mkdir -p "$src/dir"
    echo "Y" > "$src/dir/file.txt"
    ln -s "file.txt" "$src/dir/link1"

    start_daemon "$src" "$dest"

    assert_symlink_target "$dest/dir/link1" "file.txt" \
        "link1 should exist initially"

    mv "$src/dir/link1" "$src/dir/link2"

    sleep 1

    assert_not_exists "$dest/dir/link1" "link1 should disappear in dest"
    assert_symlink_target "$dest/dir/link2" "file.txt" \
        "link2 should exist in dest with same target"

    stop_daemon
    rm -rf "$root"
}

#############
# MAIN      #
#############

tests=(
    test_delete_single_file
    test_rename_single_file
    test_remove_symlink
    test_rename_symlink
)

main() {
    for t in "${tests[@]}"; do
        "$t"
        echo "OK: $t"
        echo
    done
    echo "All extra tests passed."
}

main "$@"
