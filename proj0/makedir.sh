#!/bin/bash
set -e

# Usuwamy stare drzewo, jeśli istnieje
rm -rf test_src
mkdir -p test_src

# -------------------------------------
# Tworzenie katalogów i plików
# -------------------------------------

mkdir -p test_src/a/b/c
mkdir -p test_src/outside
mkdir -p test_src/emptydir

echo "Hello from root"     > test_src/root.txt
echo "File in a"           > test_src/a/file_a.txt
echo "File in b"           > test_src/a/b/file_b.txt
echo "Nested file"         > test_src/a/b/c/file_c.txt
echo "Outside file"        > test_src/outside/out.txt

# -------------------------------------
# Symlinks względne
# -------------------------------------

# 1) względny link do pliku w tym samym drzewie
ln -s ../../root.txt test_src/a/b/link_to_root_rel

# 2) względny link do pliku w poddrzewie
ln -s c/file_c.txt test_src/a/b/link_to_c_rel

# 3) względny link *poza source*
ln -s ../outside/out.txt test_src/a/link_to_outside_rel

# -------------------------------------
# Symlinks absolutne
# -------------------------------------

abs_src="$(realpath test_src)"

# 4) absolutny link *do środka source*  → powinien zostać poprawnie przemapowany przez Twój program
ln -s "${abs_src}/a/b/c/file_c.txt" test_src/link_abs_inside

# 5) absolutny link *poza source* → musi zostać skopiowany bez zmian
ln -s "/etc/passwd" test_src/link_abs_outside

# 6) symlink do katalogu
ln -s "${abs_src}/a/b" test_src/link_abs_dir_inside

# -------------------------------------
echo "Test directory 'test_src' generated."
tree test_src
