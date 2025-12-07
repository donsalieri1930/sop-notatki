import unittest
import subprocess
import os
import shutil
import time

# --- Konfiguracja ścieżek ---
# Wymagane, aby testować logikę ścieżek bezwzględnych w C
SRC_ROOT = os.path.abspath("./test_src_temp")
DEST_ROOT = os.path.abspath("./test_backup_temp")
PROGRAM_PATH = "./p4" 

# Czas oczekiwania w sekundach, potrzebny, aby inotify przetworzył zdarzenia
INOTIFY_LAG = 0.5 

class TestFileSynchronizer(unittest.TestCase):

    def setUp(self):
        """Tworzy czyste katalogi źródłowe i docelowe przed każdym testem."""
        
        # 1. Czyszczenie starych katalogów
        if os.path.exists(SRC_ROOT):
            shutil.rmtree(SRC_ROOT)
        if os.path.exists(DEST_ROOT):
            shutil.rmtree(DEST_ROOT)
            
        # 2. Tworzenie nowych katalogów
        os.makedirs(SRC_ROOT, exist_ok=True)
        os.makedirs(DEST_ROOT, exist_ok=True)

    def tearDown(self):
        """Usuwa katalogi po każdym teście."""
        # W przypadku błędu testu, można zostawić katalogi do debugowania
        if os.path.exists(SRC_ROOT):
            shutil.rmtree(SRC_ROOT)
        if os.path.exists(DEST_ROOT):
            shutil.rmtree(DEST_ROOT)

    def run_synchro_in_background(self):
        """Uruchamia program C w tle i zwraca obiekt procesu."""
        
        # Uruchomienie programu i przekierowanie wyjścia/błędu
        return subprocess.Popen(
            [PROGRAM_PATH, SRC_ROOT, DEST_ROOT],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        
    # --- TESTY ---

    def test_01_initial_backup(self):
        """Testuje początkowe kopiowanie plików i symlinków."""
        
        # Przygotowanie: Utwórz plik i symlink w źródle
        test_file_path = os.path.join(SRC_ROOT, "config.txt")
        test_link_path = os.path.join(SRC_ROOT, "link_to_config")
        
        with open(test_file_path, "w") as f:
            f.write("Initial data.")
            
        # Tworzymy symlink bezwzględny WSKAŻUJĄCY WEWNĄTRZ monitorowanego katalogu
        os.symlink(test_file_path, test_link_path) 
        
        # 1. Uruchom program C
        process = self.run_synchro_in_background()
        
        # 2. Oczekuj na zakończenie fazy inicjalnej (program powinien być teraz w pętli inotify)
        time.sleep(INOTIFY_LAG) 
        
        # 3. Weryfikacja
        
        # Sprawdź skopiowany plik
        backup_file = os.path.join(DEST_ROOT, "config.txt")
        self.assertTrue(os.path.exists(backup_file), "Plik regularny nie został skopiowany.")
        with open(backup_file, "r") as f:
            self.assertEqual(f.read(), "Initial data.")
            
        # Sprawdź przepisany symlink
        backup_link = os.path.join(DEST_ROOT, "link_to_config")
        self.assertTrue(os.path.islink(backup_link), "Symlink nie został skopiowany.")
        
        # Oczekiwana ścieżka docelowa w backupie musi wskazywać na KOPIĘ:
        expected_target_in_backup = os.path.join(DEST_ROOT, "config.txt")
        
        # Weryfikacja, czy ścieżka bezwzględna została PRZEPISANA poprawnie
        target_path = os.path.realpath(backup_link)
        self.assertEqual(target_path, expected_target_in_backup, "Cel symlinka nie został poprawnie przepisany.")
        
        # Zakończenie procesu
        process.terminate()
        process.wait()

    def test_02_realtime_modification_and_deletion(self):
        """Testuje synchronizację zmian po starcie programu."""
        
        # 1. Przygotowanie: Uruchom program C (wykona się inicjalny backup)
        process = self.run_synchro_in_background()
        time.sleep(INOTIFY_LAG) 
        
        test_file_path = os.path.join(SRC_ROOT, "live_file.txt")
        backup_file = os.path.join(DEST_ROOT, "live_file.txt")

        # --- A. Test Modyfikacji i Tworzenia ---
        
        # Tworzymy nowy plik, powinno wywołać IN_CREATE + IN_CLOSE_WRITE
        with open(test_file_path, "w") as f:
            f.write("Nowa tresc.")
            
        time.sleep(INOTIFY_LAG) 
        self.assertTrue(os.path.exists(backup_file), "Plik nie został zsynchronizowany po utworzeniu.")
        
        # Modyfikujemy plik, powinno wywołać IN_CLOSE_WRITE
        with open(test_file_path, "a") as f:
            f.write(" Dodatkowa tresc.")
            
        time.sleep(INOTIFY_LAG) 
        with open(backup_file, "r") as f:
            self.assertEqual(f.read(), "Nowa tresc. Dodatkowa tresc.", "Zawartość nie została zsynchronizowana.")

        # --- B. Test Usuwania ---
        
        # Usuwamy plik, powinno wywołać IN_DELETE
        os.remove(test_file_path)
        
        time.sleep(INOTIFY_LAG) 
        # Ważne: to testuje, czy NIE ZAWIEDZIE w bloku remove(dest_fpath)
        self.assertFalse(os.path.exists(backup_file), "Plik nie został usunięty z backupu.")

        # Zakończenie procesu
        process.terminate()
        process.wait()


if __name__ == '__main__':
    # Aby upewnić się, że program p4 istnieje
    if not os.path.exists(PROGRAM_PATH):
        print(f"BŁĄD: Nie znaleziono programu '{PROGRAM_PATH}'. Upewnij się, że został skompilowany.")
    else:
        unittest.main()