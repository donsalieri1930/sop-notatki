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
# Kopia początkowa plików, katalogów i symlinków (rel, abs do src, abs poza src)
test_initial_copy_with_symlinks() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"          # dest NIE istnieje

    # Struktura źródłowa
    mkdir -p "$src/dir/subdir"
    echo "hello" > "$src/dir/file.txt"
    echo "other" > "$src/other.txt"

    # symlinki relatywne
    ln -s "file.txt" "$src/dir/link_rel_file"
    ln -s "../other.txt" "$src/dir/subdir/link_rel_outside"

    # symlink absolutny do wnętrza SRC
    ln -s "$src/dir/file.txt" "$src/link_abs_inside_file"

    # symlink absolutny poza SRC
    ln -s "/etc/passwd" "$src/link_abs_outside"

    start_daemon "$src" "$dest"

    # pliki
    assert_exists "$dest/dir/file.txt" "file.txt should be copied"
    assert_exists "$dest/other.txt"    "other.txt should be copied"
    assert_eq "hello" "$(cat "$dest/dir/file.txt")" "file.txt content mismatch"
    assert_eq "other" "$(cat "$dest/other.txt")"    "other.txt content mismatch"

    # symlink relatywny -> target bez zmian
    assert_symlink_target "$dest/dir/link_rel_file" "file.txt" \
        "relative symlink should keep target unchanged"

    assert_symlink_target "$dest/dir/subdir/link_rel_outside" "../other.txt" \
        "relative symlink outside should keep target unchanged"

    # symlink absolutny do SRC -> przepisać na DEST
    assert_symlink_target "$dest/link_abs_inside_file" "$dest/dir/file.txt" \
        "absolute symlink into src should be rewritten to dest"

    # symlink absolutny poza SRC -> zostaje jak był
    assert_symlink_target "$dest/link_abs_outside" "/etc/passwd" \
        "absolute symlink outside src should stay unchanged"

    stop_daemon
    rm -rf "$root"
}

# TEST 2:
# Symlink absolutny do katalogu w SRC -> przepisać na DEST
test_absolute_dir_symlink_rewrite() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"

    mkdir -p "$src/tree/sub"
    echo "x" > "$src/tree/sub/a.txt"

    ln -s "$src/tree" "$src/link_abs_tree"

    start_daemon "$src" "$dest"

    assert_symlink_target "$dest/link_abs_tree" "$dest/tree" \
        "absolute symlink to dir inside src should be rewritten"
    assert_exists "$dest/tree/sub/a.txt" "tree/sub/a.txt should exist in dest"

    stop_daemon
    rm -rf "$root"
}

# TEST 3:
# Dodawanie nowego poddrzewa po starcie (katalogi, pliki, symlink relatywny)
test_monitor_add_subtree() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"

    mkdir -p "$src/base"
    echo "base" > "$src/base/file0.txt"

    start_daemon "$src" "$dest"

    assert_exists "$dest/base/file0.txt" "base/file0.txt should be copied initially"

    # nowe poddrzewo
    mkdir -p "$src/new/sub/dir"
    echo "N1" > "$src/new/sub/dir/f1.txt"
    echo "N2" > "$src/new/sub/dir/f2.txt"
    ln -s "f1.txt" "$src/new/sub/dir/link_rel"

    sleep 1

    assert_exists "$dest/new/sub/dir/f1.txt" "f1.txt should be mirrored"
    assert_exists "$dest/new/sub/dir/f2.txt" "f2.txt should be mirrored"
    assert_eq "N1" "$(cat "$dest/new/sub/dir/f1.txt")" "N1 content mismatch"
    assert_eq "N2" "$(cat "$dest/new/sub/dir/f2.txt")" "N2 content mismatch"

    assert_symlink_target "$dest/new/sub/dir/link_rel" "f1.txt" \
        "relative symlink in new subtree should keep target"

    stop_daemon
    rm -rf "$root"
}

# TEST 4:
# Przeniesienie całego poddrzewa (mv dirA dirB)
test_monitor_move_subtree() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"

    mkdir -p "$src/dirA/sub"
    echo "aaa" > "$src/dirA/sub/a.txt"

    start_daemon "$src" "$dest"

    assert_exists "$dest/dirA/sub/a.txt" "dirA/sub/a.txt should exist initially"

    mv "$src/dirA" "$src/dirB"

    sleep 1

    assert_not_exists "$dest/dirA" "dirA should disappear in dest after move"
    assert_exists "$dest/dirB/sub/a.txt" "dirB/sub/a.txt should exist after move"
    assert_eq "aaa" "$(cat "$dest/dirB/sub/a.txt")" "content after move mismatch"

    stop_daemon
    rm -rf "$root"
}

# TEST 5:
# Usuwanie całego poddrzewa (rm -rf)
test_monitor_delete_subtree() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"

    mkdir -p "$src/tree/sub1/sub2"
    echo "to_delete" > "$src/tree/sub1/sub2/x.txt"

    start_daemon "$src" "$dest"

    assert_exists "$dest/tree/sub1/sub2/x.txt" "subtree should be copied initially"

    rm -rf "$src/tree"

    sleep 1

    assert_not_exists "$dest/tree" "tree subtree should be removed from dest"

    stop_daemon
    rm -rf "$root"
}

# TEST 6:
# Nowy symlink absolutny do wnętrza SRC po starcie
test_monitor_new_absolute_symlink_inside_src() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"

    echo "val" > "$src/file.txt"

    start_daemon "$src" "$dest"

    assert_exists "$dest/file.txt" "file.txt should be copied initially"

    ln -s "$src/file.txt" "$src/link_new_abs"

    sleep 1

    assert_symlink_target "$dest/link_new_abs" "$dest/file.txt" \
        "new absolute symlink into src should be rewritten to dest"

    stop_daemon
    rm -rf "$root"
}

# TEST 7:
# Modyfikacja zawartości pliku (zmiana treści) po starcie
test_monitor_modify_file_content() {
    echo "Running: $FUNCNAME"

    local root src dest
    root="$(mktemp -d)"
    src="$root/src"
    dest="$root/dest"
    mkdir "$src"

    echo "old" > "$src/file.txt"

    start_daemon "$src" "$dest"

    assert_exists "$dest/file.txt" "file.txt should be copied initially"
    assert_eq "old" "$(cat "$dest/file.txt")" "initial content mismatch"

    # modyfikacja pliku w SRC
    echo "new" > "$src/file.txt"

    sleep 1

    assert_eq "new" "$(cat "$dest/file.txt")" "modified content should be mirrored"

    stop_daemon
    rm -rf "$root"
}

#############
# MAIN      #
#############

tests=(
    test_initial_copy_with_symlinks
    test_absolute_dir_symlink_rewrite
    test_monitor_add_subtree
    test_monitor_move_subtree
    test_monitor_delete_subtree
    test_monitor_new_absolute_symlink_inside_src
    test_monitor_modify_file_content
)

main() {
    for t in "${tests[@]}"; do
        "$t"
        echo "OK: $t"
        echo
    done
    echo "All tests passed."
}

main "$@"
