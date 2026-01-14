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

## 5. Testy i weryfikacja wymagań

Zgodnie z wymaganiami przeprowadzono serię testów weryfikujących logikę:

| Nr | Nazwa Testu | Opis scenariusza | Oczekiwany rezultat |
|----|-------------|------------------|---------------------|
| 1 | **Limit Taśmy (K)** | Uruchomienie z małym K (np. 2). Pracownicy próbują wrzucić 5 paczek. | Pracownicy blokują się po wrzuceniu 2 paczek, czekając aż Ciężarówka zwolni miejsce. (Brak przepełnienia bufora). |
| 2 | **Maksymalna Waga (M)** | Ustawienie limitu wagi M mniejszego niż waga paczki C. | Pracownik próbujący wrzucić paczkę C czeka, aż suma wag na taśmie spadnie poniżej progu pozwalającego na dodanie C. |
| 3 | **Odjazd i wymiana** | Ciężarówka zapełnia swoją pojemność (V) lub wagę (W). | Ciężarówka komunikuje odjazd, znika z systemu, a po chwili pojawia się nowa (lub ta sama wraca) z pustą paką. |
| 4 | **Priorytet Express (P4)** | Taśma pełna (K/K). Użytkownik wysyła komendę "Express Load". | P4 czeka, aż zwolni się *jedno* miejsce, po czym zajmuje je *przed* pracownikami standardowymi (dzięki priorytetowemu dostępowi do semafora lub logice Dispatchera). |
| 5 | **Wymuszony odjazd** | Ciężarówka jest w połowie pełna. Użytkownik wysyła sygnał 1 (SIGUSR1). | Ciężarówka natychmiast przerywa załadunek i odjeżdża z obecnym stanem towaru. |

## 6. Linki do kodu (GitHub)

Poniżej znajdują się odnośniki do kluczowych fragmentów kodu realizujących wymagania systemowe:

* **a. Tworzenie i obsługa plików (open, dup2):**
    * [Przekierowanie logów w main.c]([LINK_DO_TWOJEGO_REPO]/blob/main/src/main.c#L[LINIA])
* **b. Tworzenie procesów (fork, exec, exit, wait):**
    * [Pętla tworząca workerów w main.c]([LINK_DO_TWOJEGO_REPO]/blob/main/src/main.c#L[LINIA])
* **c. Obsługa sygnałów (kill, sigaction):**
    * [Handler sygnałów w truck.c]([LINK_DO_TWOJEGO_REPO]/blob/main/src/truck.c#L[LINIA])
    * [Wysłanie sygnału przez Dispatchera]([LINK_DO_TWOJEGO_REPO]/blob/main/src/main.c#L[LINIA])
* **d. Synchronizacja procesów (semafory System V):**
    * [Inicjalizacja semaforów]([LINK_DO_TWOJEGO_REPO]/blob/main/src/common/sem_wrapper.c#L[LINIA])
    * [Operacje P/V (wait/signal)]([LINK_DO_TWOJEGO_REPO]/blob/main/src/common/sem_wrapper.c#L[LINIA])
* **e. Segmenty pamięci dzielonej (shmget, shmat):**
    * [Struktura SharedState w common.h]([LINK_DO_TWOJEGO_REPO]/blob/main/src/common/common.h#L[LINIA])
    * [Dołączenie pamięci w worker_std.c]([LINK_DO_TWOJEGO_REPO]/blob/main/src/worker_std.c#L[LINIA])
* **f. Inne (Wątki/Kolejki/Potoki - jeśli użyto):**
    * *(W tym projekcie nie używano wątków ani kolejek komunikatów, zgodnie z wyborem semaforów + SHM)*.
