#!/usr/bin/env bash
set -euo pipefail

PROG=./p4        # Twój program
SRC=test_src     # katalog źródłowy
DEST=backup      # katalog backupu

# 0. Sprzątanie po poprzednich testach
rm -rf "$SRC" "$DEST"

mkdir -p "$SRC"
mkdir -p "$DEST"   # bo w C używasz realpath("backup"), więc musi istnieć

# 1. Zbuduj drzewo testowe w test_src/
#
# test_src/
#   dir1/file1
#   dir2/file2
#   file_root
#   link_inside_abs   -> ABS(test_src/dir1/file1)
#   link_outside_abs  -> ABS(outside.txt)
#   link_rel_inside   -> dir2/file2
#   link_rel_outside  -> ../outside.txt

mkdir -p "$SRC/dir1" "$SRC/dir2"

echo "one"   > "$SRC/dir1/file1"
echo "two"   > "$SRC/dir2/file2"
echo "root"  > "$SRC/file_root"

OUTSIDE=outside.txt
echo "outside" > "$OUTSIDE"

SRC_ABS=$(realpath "$SRC")
OUTSIDE_ABS=$(realpath "$OUTSIDE")

ln -s "$SRC_ABS/dir1/file1" "$SRC/link_inside_abs"
ln -s "$OUTSIDE_ABS"        "$SRC/link_outside_abs"
ln -s "dir2/file2"          "$SRC/link_rel_inside"
ln -s "../outside.txt"      "$SRC/link_rel_outside"

echo "==== POCZĄTKOWE ŹRÓDŁO ===="
find "$SRC" -maxdepth 3 -printf '%y %p -> %l\n'

# 2. Start programu w tle
"$PROG" "$SRC" &
BACKUP_PID=$!

# dajemy programowi chwilę na początkowy backup
sleep 1

echo
echo "==== POCZĄTKOWY BACKUP ===="
find "$DEST" -maxdepth 4 -printf '%y %p -> %l\n'

echo
echo "==== TEST 1: przeniesienie katalogu dir1 -> moved_dir1 ===="
mv "$SRC/dir1" "$SRC/moved_dir1"
sleep 1

echo "Źródło:"
find "$SRC" -maxdepth 3 -printf '%y %p -> %l\n'
echo
echo "Backup:"
find "$DEST" -maxdepth 4 -printf '%y %p -> %l\n'

echo
echo "==== TEST 2: przeniesienie pliku dir2/file2 -> dir2/file2_renamed ===="
mv "$SRC/dir2/file2" "$SRC/dir2/file2_renamed"
sleep 1

echo "Źródło:"
find "$SRC" -maxdepth 3 -printf '%y %p -> %l\n'
echo
echo "Backup:"
find "$DEST" -maxdepth 4 -printf '%y %p -> %l\n'

echo
echo "==== TEST 3: podmiana link_inside_abs na inny cel wewnątrz src ===="
rm "$SRC/link_inside_abs"
ln -s "$SRC_ABS/moved_dir1/file1" "$SRC/link_inside_abs"
sleep 1

echo "Źródło:"
find "$SRC" -maxdepth 3 -printf '%y %p -> %l\n'
echo
echo "Backup:"
find "$DEST" -maxdepth 4 -printf '%y %p -> %l\n'

# 3. Zatrzymujemy program
kill "$BACKUP_PID" 2>/dev/null || true
wait "$BACKUP_PID" 2>/dev/null || true

echo
echo "==== KONIEC TESTU ===="
