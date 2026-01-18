# Raport z projektu: Systemy Operacyjne

**Autor:** Mikołaj Kosiorek\
**Nr albumu:** 155281\
**Temat:** Temat 10 - Magazyn firmy spedycyjnej

---

## 1. Założenia projektowe

Celem projektu było stworzenie symulacji działania magazynu firmy spedycyjnej w środowisku systemu Linux, wykorzystując mechanizmy programowania współbieżnego i komunikacji międzyprocesowej (IPC). Zgodnie z wymaganiami, unikano rozwiązań scentralizowanych na poziomie logiki biznesowej – każdy proces (Ciężarówka, Pracownik) jest osobnym procesem podejmującym autonomiczne decyzje w oparciu o stan pamięci dzielonej.

**Główne założenia:**
* **Architektura:** Wieloprocesowa, oparta na funkcji `fork()` i `exec()`. Proces główny (`Dispatcher`) pełni rolę orkiestratora cyklu życia, ale nie pośredniczy w przekazywaniu danych.
* **Komunikacja:** Pamięć współdzielona (Shared Memory) przechowuje stan taśmy (bufor kołowy) oraz flagi synchronizacyjne.
* **Synchronizacja:** Zestaw semaforów Systemu V (Semaphores) realizuje problem producenta i konsumenta, chroniąc dostęp do sekcji krytycznych (taśma) oraz sygnalizując stany pełny/pusty.
* **Logowanie:** Wyjście standardowe procesów potomnych jest przekierowane funkcją `dup2()` do pliku `simulation.log`, aby zapewnić czytelność i uniknąć wyścigów o zasób `stdout` terminala.

## 2. Opis implementacji

Kod projektu został podzielony na moduły zgodnie z zasadą separacji odpowiedzialności:

* **Dispatcher (`main.c`):** Inicjalizuje zasoby IPC, tworzy procesy potomne (N Ciężarówek, 3 Pracowników Standardowych, 1 Express) i obsługuje interfejs CLI (komendy użytkownika). Reaguje na sygnały `SIGINT` (bezpieczne zamknięcie).
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
    * *Rozwiązanie:* Wprowadzono flagę kompilacji CMake `-DSIM_DELAY_MS`, która wstrzykuje opóźnienia (`nanosleep`) do pętli procesów, umożliwiając analizę logów w czasie rzeczywistym.

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
    * [Pętle tworzące procesy potomne (fork, exec)](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/main.c#L158-L198)
    * [Oczekiwanie na zakończenie pracy procesów (wait)](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/main.c#L269)
* **d. Obsługa sygnałów (kill, sigaction):**
    * [Handler sygnałów w truck.c](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/truck.c#L28-L47)
    * [Wysłanie sygnału przez Dispatchera do truck](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/main.c#L217-L227)
    * [Handler sygnałów w worker_express.c](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/worker_express.c#L28-L46)
    * [Wysyłanie sygnału przez Dispatchera do worker_express](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/main.c#L235-L241)
    * [Zamknięcie symulacji przez usunięcie procesów](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/main.c#L242-L261)
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
* **h. Inne:**
    * [Możliwość zdefiniowania opóźnienia w celu obserwacji logów](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/CMakeLists.txt#L5-L8)
    * [Przykładowy fragment kolorowania wyjścia](https://github.com/MikolajKos/shipping-warehouse-simulator/blob/20851eee70aa8c65a77a145cc3c6a53e37b08bb6/src/worker_std.c#L91-L94)




