# Raport z projektu: Systemy Operacyjne
[Dokumentacja Online](https://mkosiorek.pl/docs/warehouse_sim/files.html)

**Autor:** Mikołaj Kosiorek\
**Nr albumu:** 155281\
**Temat:** Temat 10 - Magazyn firmy spedycyjnej

---

## Środowisko deweloperskie i wymagania

Projekt został zaimplementowany i przetestowany w środowisku systemu Linux. Do poprawnej kompilacji i uruchomienia symulacji wymagane są następujące narzędzia oraz biblioteki systemowe zgodne ze standardem POSIX.

**Specyfikacja środowiska:**
* **System operacyjny:** Linux (testowano na dystrybucji Linux Mint/Ubuntu).
* **Kompilator:** GCC (GNU Compiler Collection) ze wsparciem dla standardu **C11** (flaga `-std=gnu11`).
* **System budowania:** **CMake** (wersja min. 3.25) – służy do automatyzacji procesu kompilacji.
* **Biblioteki:**
    * `pthread` (POSIX Threads) – wymagana do GoogleTests.
    * Standardowe biblioteki systemowe: `<sys/ipc.h>`, `<sys/shm.h>`, `<sys/sem.h>`, `<sys/wait.h>`, `<unistd.h>`, `<signal.h>`.
* **Edytor kodu (IDE):** **GNU Emacs** – środowisko wykorzystane do implementacji projektu.
> **Uwaga:** Projekt wykorzystuje moduł `FetchContent`. Podczas pierwszego uruchomienia polecenia `cmake ..`, system automatycznie pobierze i skompiluje bibliotekę **GoogleTest** z repozytorium GitHub. Wymagane jest aktywne połączenie z internetem.

**Instalacja zależności (Debian/Ubuntu/Mint):**
Aby przygotować czyste środowisko do pracy, należy zainstalować kompilator oraz narzędzie CMake:
```bash
sudo apt update
sudo apt install build-essential cmake git
```

## Instrukcje budowania
Poniżej znajduje się kompletna instrukcja zawierająca sekwencję instrukcji niezbędnych do budowy projektu.
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

**Opcjonalnie:** Implementacja programu pozwala na ustawienie domyślnego opóźnienia pętli procesów w milisekundach, dla łatwiejszego odczytu logów
```bash
cd build
cmake .. -DSIM_DELAY_MS=3000 # Ustawienie 3 sekund opóźnienia
cmake --build .
```

## Uruchamianie projektu
Symulacje można uruchomić z katalogu build/src. Należy podać odpowiednie argumenty uruchomieniowe:
```bash
cd build/src
./warehouse_dispatcher <N_Trucks> <K_BeltCapacity> <M_MaxBeltWeight> <W_TruckWeight> <V_TruckVolume>
```

**Przykład:**
```bash
./warehouse_dispatcher 3 10 500 100 50
```

**Interaktywna obsługa komend**
Po uruchomieniu program nasłuchuje komend na standardowym wejściu stdin:
- 1: Force Departure - Sygnał ten zmusza ciężarówkę do natychmiastowego opuszczenia doku i dostarczenia przesyłek.
- 2: Express Load - Obudzenie procesu pracownika ekspresowego, załadunek kilku paczek bezpośrednio do ciężarówki.
- 3: Shutdown - Bezpieczne zamykanie symulacji, wszystkie procesy są zamykane a ciężarówki rozwożą ostatnie produkty.

## Testowanie projektu

Po zbudowaniu projektu w katalogu build możliwe jest przeprowadzenie testów jednostowych i integracyjnych
```bash
cd build
ctest --output-on-failure
# Dla lepszej wizualizacji testów można je wykonać również z katalogu tests
cd build/tests
./test_trucks
```

## 1. Założenia projektowe

Celem projektu było stworzenie symulacji działania magazynu firmy spedycyjnej w środowisku systemu Linux, wykorzystując mechanizmy programowania współbieżnego i komunikacji międzyprocesowej (IPC). Zgodnie z wymaganiami, unikano rozwiązań scentralizowanych na poziomie logiki biznesowej – każdy proces (Ciężarówka, Pracownik) jest osobnym procesem podejmującym autonomiczne decyzje w oparciu o stan pamięci dzielonej.

**Główne założenia:**
* **Architektura:** Wieloprocesowa, oparta na funkcji `fork()` i `exec()`. Proces główny (`Dispatcher`) pełni rolę orkiestratora cyklu życia, ale nie pośredniczy w przekazywaniu danych.
* **Komunikacja:** Pamięć współdzielona (Shared Memory) przechowuje stan taśmy (bufor kołowy) oraz flagi synchronizacyjne.
* **Synchronizacja:** Zestaw semaforów Systemu V (Semaphores) realizuje problem producenta i konsumenta, chroniąc dostęp do sekcji krytycznych (taśma) oraz sygnalizując stany pełny/pusty.
* **Logowanie:** Wyjście standardowe procesów potomnych jest przekierowane funkcją `dup2()` do pliku `simulation.log`, aby zapewnić czytelność i uniknąć wyścigów o zasób `stdout` terminala.

## 2. Opis implementacji

Kod projektu został podzielony na moduły zgodnie z zasadą separacji odpowiedzialności:

* **Dispatcher (`main.c`):** Inicjalizuje zasoby IPC, tworzy procesy potomne (N Ciężarówek, 3 Pracowników Standardowych, 1 Express) i obsługuje interfejs CLI (komendy użytkownika).
* **Pracownicy (`worker_std.c`, `worker_express.c`):** Procesy cyklicznie generujące paczki.
    * *Worker Standard:* Produkuje paczki A, B, C zgodnie z ograniczeniami wagi taśmy (M) i pojemności (K).
    * *Worker Express:* Czeka na sygnał `SIGUSR1` od Dispatchera, aby załadować priorytetową paczkę, omijając standardową kolejkę (jeśli jest miejsce fizyczne).
* **Ciężarówki (`truck.c`):** Procesy konsumujące paczki z taśmy. Każda ciężarówka zna swoją ładowność (W) i pojemność (V). Pobiera paczki w kolejności FIFO. Po zapełnieniu lub otrzymaniu sygnału `SIGUSR1` (wymuszony odjazd), proces kończy pracę (odjazd), symuluje trasę (`sleep`) i wraca (nowy proces lub pętla).
* **Mechanizm Taśmy:** Zaimplementowany jako bufor kołowy w pamięci współdzielonej, chroniony semaforem MUTEX.

## 3. Napotkane problemy i rozwiązania

Podczas realizacji projektu napotkano następujące wyzwania:

1.  **Problem buforowania wyjścia (IO Buffering):**
    * *Objaw:* Logi w pliku pojawiały się "blokami" (np. 20 linii od ciężarówki, potem 30 od pracownika), co uniemożliwiało śledzenie chronologii zdarzeń.
    * *Przyczyna:* Przekierowanie `dup2` do pliku włącza pełne buforowanie strumienia `stdout`.
    * *Rozwiązanie:* Zastosowano `setbuf(stdout, NULL)` w każdym procesie potomnym, wymuszając natychmiastowy zapis do pliku.

2.  **Szybkość symulacji:**
    * *Objaw:* Procesy wykonywały się zbyt szybko dla ludzkiego oka.
    * *Rozwiązanie:* Wprowadzono flagę kompilacji CMake `-DSIM_DELAY_MS`, która wstrzykuje opóźnienia (`usleep`) do pętli procesów, umożliwiając analizę logów w czasie rzeczywistym.

3.  **Ryzyko zakleszczenia (Deadlock) przy zatrzymywaniu:**
    * *Objaw:* Przy sygnale kończącym, procesy wiszące na semaforach (np. czekające na paczkę) nie kończyły się.
    * *Rozwiązanie:* Dispatcher wysyła `SIGTERM` do wszystkich dzieci, co przerywa operacje blokujące semaforów, a następnie wykonuje `wait()` dla czystego zamknięcia.

## 4. Elementy specjalne (wyróżniające)

* **Konfiguracja przez CMake:** Możliwość sterowania prędkością symulacji bez zmiany kodu źródłowego.
* **Testy integracyjne:** Zastosowanie frameworka **GoogleTest** do weryfikacji logiki biznesowej (np. czy ciężarówka nie zabiera paczki przekraczającej jej udźwig).
* **Kolorowanie logów:** Użycie kodów ANSI w pliku logów, co po otwarciu przez `tail -f` daje czytelny, kolorowy podgląd sytuacji.
* **Dokumentacja Doxygen:** Implementacja profesjonalnego generatora dokumentacji.
* **Publikacja Dokumentacji (FTP):** Umiesznienie dokumentacji projektowej przez FTP na mojej prywatnej domenie [Dokumentacja Online](https://mkosiorek.pl/docs/warehouse_sim/files.html).

## Tabela 5. Scenariusze testowe i oczekiwane rezultaty

| Nr | Plik źródłowy | Nazwa Testu (Funkcja) | Opis scenariusza | Oczekiwany rezultat |
|:--:|:---|:---|:---|:---|
| **1** | `test_worker_std.cpp` | `PlacesPackagesIfSpaceEmpty` | Pracownik standardowy uruchamia się, gdy na taśmie są wolne sloty (`SEM_EMPTY > 0`). | Waga taśmy wzrasta powyżej `0.0` (paczka została poprawnie dodana do bufora). |
| **2** | `test_worker_std.cpp` | `BeltsWeightLimitReachedCantPlace` | Ustawienie limitu wagi taśmy (M) na `0.0`. Pracownik próbuje dodać nową paczkę. | Waga taśmy pozostaje `0.0`. Pracownik nie umieszcza paczki, respektując limit M. |
| **3** | `test_worker_std.cpp` | `BlockIfBeltIsFull` | Brak wolnych miejsc na taśmie (`SEM_EMPTY` ustawiony na 0). Uruchomienie procesu pracownika. | Licznik paczek (`current_count`) nie zmienia się – proces pracownika zostaje zablokowany na semaforze. |
| **4** | `test_worker_express.cpp` | `IgnoresSignalWhenNoTruck` | Wysłanie sygnału `SIGUSR1` do pracownika Express, gdy w doku brak ciężarówki (`truck_docked = 0`). | Ładunek ciężarówki pozostaje `0.0`. Pracownik ignoruje sygnał i nie ładuje towaru "w próżnię". |
| **5** | `test_worker_express.cpp` | `LoadsPackagesWhenTruckDocked` | Wysłanie sygnału `SIGUSR1` do pracownika Express, gdy ciężarówka jest poprawnie zadokowana. | Ładunek ciężarówki (`current_truck_load`) oraz zajęta objętość wzrastają powyżej `0`. |
| **6** | `test_worker_express.cpp` | `SkipPackagesIfLimitReached` | Pracownik Express próbuje załadować paczkę, która przekracza pozostałą ładowność ciężarówki. | Ładunek ciężarówki nie przekracza maksymalnej pojemności (`truck_capacity_W`). Nadmiarowa paczka jest pomijana. |
| **7** | `test_truck.cpp` | `LoadingAllPackages` | Na taśmie znajduje się 5 paczek. Ciężarówka podjeżdża do doku. | Ciężarówka pobiera wszystkie paczki. Jej końcowy ładunek jest równy sumie wag paczek z taśmy. |
| **8** | `test_truck.cpp` | `PkgLoadingAndDeparture` | Ciężarówka zapełnia się, odjeżdża (symulacja czasu dostawy) i wraca do kolejki. | Po powrocie zmienne współdzielone ciężarówki (`load`, `vol`) są resetowane do `0.0` (ciężarówka jest pusta). |
| **9** | `test_truck.cpp` | `RespectsVolumeLimits` | Paczki na taśmie mieszczą się w limicie wagowym, ale przekraczają limit objętości (V). | Załadowana zostaje tylko ta część paczek, która mieści się w limicie objętości. Reszta pozostaje na taśmie. |
| **10** | `test_truck.cpp` | `ForcedDepartureBySignal` | Wysłanie sygnału `SIGUSR1` (wymuszony odjazd) w trakcie trwania załadunku (gdy na taśmie wciąż są paczki). | Ciężarówka przerywa pętlę ładowania, zwalnia dok (`truck_docked = 0`) i odjeżdża z częściowym ładunkiem. |
| **11** | `test_truck.cpp` | `SkipOversizedPackage` | Na taśmie (jako pierwsza) znajduje się paczka cięższa niż całkowita ładowność ciężarówki. | Ciężarówka nie ładuje paczki (`load = 0.0`), pozostawiając ją na taśmie. Algorytm "Peek & Check" działa poprawnie. |
| **12** | `test_truck.cpp` | `NextTruckLoadsFirstItem` | Pierwsza ciężarówka nie zabiera paczki z powodu braku miejsca/odjazdu. Podjeżdża druga ciężarówka. | Druga ciężarówka pobiera tę samą paczkę (zachowanie kolejki FIFO bufora cyklicznego). |
| **13** | `test_utils.cpp` | `GeneratedWeightIsWithinBounds` | Test jednostkowy funkcji generującej wagi dla typu paczki `PKG_C`. | Wygenerowana waga mieści się w zdefiniowanym przedziale `[0.1, 25.0]`. |
| **14** | `test_utils.cpp` | `VolumeZeroForUnknownType` | Przekazanie nieprawidłowego identyfikatora typu paczki do funkcji obliczającej objętość. | Funkcja zwraca bezpieczną wartość `0.0` (obsługa błędów). |

## 6. Linki do kodu (GitHub)

Poniżej znajdują się odnośniki do kluczowych fragmentów kodu realizujących wymagania systemowe:

* **a. Walidacja poprawności wprowadzanych danych:**
    * [Walidacja wprowadzanych parametrów](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/main.c#L103-L130)
    * [Sprawdzenie, czy liczba procesów nie przekroczy systemowego limitu](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/main.c#L119-L125)
* **b. Tworzenie i obsługa plików (open, dup2):**
    * [Tworzenie deskryptora dla wyjścia logów](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/main.c#L136)
    * [Przekierowanie logów w main.c](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/main.c#L164)
* **c. Tworzenie procesów (fork, exec, exit, wait):**
    * [Pętle tworzące procesy potomne (fork, exec)](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/main.c#L165-L214)
    * [Oczekiwanie na zakończenie pracy procesów z wypisywaniem (wait)](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/main.c#L312-L327)
* **d. Obsługa sygnałów (kill, sigaction):**
    * [Handler sygnałów w truck.c](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/truck.c#L28-L47)
    * [Wysłanie sygnału przez Dispatchera do truck](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/main.c#L255-L272)
    * [Handler sygnałów w worker_express.c](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/worker_express.c#L28-L46)
    * [Wysyłanie sygnału przez Dispatchera do worker_express](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/main.c#L273-L279)
    * [Zamknięcie symulacji przez usunięcie procesów](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/main.c#L280-L305)
* **e. Synchronizacja procesów (semafory System V):**
    * [Inicjalizacja semaforów w main.c](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/main.c#L70-L75)
    * [Zabezpieczenie przed blokowaniem procesu truck](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/truck.c#L155-L168)
    * [Operacje P/V (wait/signal)](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/common/sem_wrapper.c#L10-L21)
    * [Statyczna biblioteka pomocnicza (sem_wrapper)](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/common/sem_wrapper.h)
* **f. Segmenty pamięci dzielonej (shmget, shmat):**
    * [Struktura SharedState w common.h](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/common/common.h#L97-L122)
    * [Dołączenie pamięci w worker_std.c](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/worker_std.c#L47-L54)
    * [Statyczna biblioteka pomocnicza (shm_wrapper)](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/common/shm_wrapper.h)
* **g. Funkcje pomocnicze utils (generowanie przesyłek, pobieranie czasu):**
    * [Plik utils.h zawierający funkcje pomocnicze](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/common/utils.h)
* **h. Zagadnienia powiązane z tematem projektu:**
    * [Ładowanie paczki do ciężarówki](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/truck.c#L203-L216)
    * [Sprawdzenie limitu ciężarówki](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/truck.c#L191-L200)
    * [Położenie paczki na taśmę (Standard Worker)](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/worker_std.c#L108-L129)
    * [Sprawdzenie limitu taśmy](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/worker_std.c#L83-L105)
    * [Położenie paczki na taśmę (Express Worker P4)](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/worker_express.c#L64-L82)
    * [Oczekiwanie Express Workera na sygnał wybudzający](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/worker_express.c#L128-L133)
    * [Miejsce naturalnego usunięcia procesów ciężarówek](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/579bb111569328b60d644c373aa45ef896d532a6/src/truck.c#L122-L129)
* **i. Inne:**
    * [Możliwość zdefiniowania opóźnienia w celu obserwacji logów](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/CMakeLists.txt#L5-L8)
    * [Przykładowy fragment kolorowania wyjścia](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/worker_std.c#L91-L94)










