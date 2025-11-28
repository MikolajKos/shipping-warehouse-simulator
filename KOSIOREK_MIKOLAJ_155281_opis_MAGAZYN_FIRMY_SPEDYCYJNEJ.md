# 📝 Opis Projektu
Projekt akademicki skupiający się na zastosowaniu procesów i wątów w symulacji magazynu firmy spedycyjnej. 

## 🔗 Link do repozytorium
[Magazyn firmy spedycyjnej](https://github.com/MikolajKos/shipping-warehouse-simulator.git)

## 🎯 Temat projektu
### Temat 10 – Magazyn firmy spedycyjnej
W magazynie przy taśmie transportowej pracuje trzech pracowników oznaczonych przez P1, P2 i P3.
Pracownicy układają na taśmę przesyłki o gabarytach odpowiednio A (64x38x8 cm), B (64x38x19
cm) i C (64x38x41 cm). Wszystkie paczki muszą mieć maksymalną wagę 25 kg (wartość losowa z
15
zakresu 0,1kg-25,0kg) – można przyjąć założenie: czym mniejsza paczka tym mniejszy ciężar. W
magazynie znajduje się również pracownik P4, który jest odpowiedzialny za załadunek przesyłek
ekspresowych. Przesyłki ekspresowe dostarczane są osobno - nie są umieszczane na taśmie.
Przesyłki ekspresowe mają wyższy priorytet – ich załadunek musi odbyć się w pierwszej kolejności.
Pakiet przesyłek ekspresowych może zawierać tylko przesyłki o gabarytach A, B i C, których waga
(pojedynczej przesyłki) jest mniejsza niż 25kg.

Na końcu taśmy stoi ciężarówka o ładowności W jednostek [kg] oraz dopuszczalnej objętości ładunku
V [m3], którą należy zawsze załadować do pełna. Wszyscy pracownicy starają się układać przesyłki
na taśmie najszybciej jak to możliwe. Taśma może przetransportować w danej chwili maksymalnie K
przesyłek. Jednocześnie jednak taśma ma ograniczony udźwig: maksymalnie M jednostek masy, tak,
że niedopuszczalne jest położenie np. samych tylko najcięższych przesyłek (K*25kg > M). Przesyłki
„zjeżdżające" z taśmy muszą od razu trafić na samochód dokładnie w takiej kolejności jak zostały
położone na taśmie. Po zapełnieniu ciężarówki na jej miejsce pojawia się natychmiast (jeżeli jest
dostępna!) nowa o ładowności W oraz dopuszczalnej objętości ładunku V. Łączna liczba ciężarówek
wynosi N. Ciężarówki rozwożą przesyłki i po czasie Ti wracają do magazynu.

Na polecenie dyspozytora (sygnał 1) ciężarówka, która w danym momencie stoi przy taśmie może
odjechać z magazynu z niepełnym ładunkiem.

Po otrzymaniu od dyspozytora polecenia (sygnał 2) pracownik P4 dostarcza do stojącej przy taśmie
ciężarówki pakiet przesyłek ekspresowych.

Po otrzymaniu od dyspozytora polecenia (sygnał 3) pracownicy kończą pracę. Ciężarówki kończą
pracę po rozwiezieniu wszystkich przesyłek.

Napisz programy symulujące działanie dyspozytora, pracowników i ciężarówek. Raport z przebiegu
symulacji zapisać w pliku (plikach) tekstowym.

## 🔍 Opis testów
Poniżej przedstawiono scenariusze testowe, które posłużą do weryfikacji poprawności działania symulacji magazynu. Testy skupiają się na krytycznych mechanizmach logiki biznesowej systemu.
1. ✅ **Test priorytetu przesyłek ekspresowych (P4 / Sygnał 2)**
   - **Cel:** Sprawdzenie, czy przesyłki od pracownika P4 są ładowane przed paczkami oczekującymi na taśmie.
   - **Scenariusz:**
     1. Taśma jest zapełniona paczkami od P1, P2, P3, które oczekują na załadunek.
     2. Dyspozytor wysyła Sygnał 2 (zlecenie dla P4).
     3. Pracownik P4 dostarcza pakiet przesyłek ekspresowych.
   - **Oczekiwany rezultat:** Ciężarówka wstrzymuje pobieranie paczek z taśmy i natychmiast ładuje pakiet ekspresowy od P4. Dopiero po załadowaniu "ekspresu", proces ładowania z taśmy jest wznawiany.
     
2. ✅ **Test zabezpieczenia przeciążenia taśmy (Parametry K i M)**
   - **Cel:** Weryfikacja, czy system blokuje dodawanie paczek na taśmę po przekroczeniu dopuszczalnej masy ($M$) lub liczby paczek ($K$).
   - **Scenariusz:**
     1. Ustawienie bardzo małego limitu udźwigu taśmy $M$ (np. równowartość wagi 3 paczek typu C).
     2. Pracownicy P1, P2, P3 próbują jednocześnie położyć na taśmę dużą liczbę ciężkich paczek.
   - **Oczekiwany rezultat:** Pracownicy muszą "czekać" (zostać zablokowani), zanim położą kolejną paczkę, dopóki suma mas paczek znajdujących się aktualnie na taśmie nie spadnie poniżej wartości $M$. Taśma nigdy nie przekracza limitu $M$ ani $K$.

3. ✅ **Test wcześniejszego odjazdu na żądanie (Sygnał 1)**
   - **Cel:** Sprawdzenie reakcji ciężarówki na polecenie dyspozytora o wymuszonym odjeździe.
   - **Scenariusz:**
     1. Podstawiona jest pusta ciężarówka o dużej pojemności ($W$ i $V$).
     2. Zostaje załadowane tylko kilka paczek (ciężarówka jest zapełniona w np. 10%).
     3. Dyspozytor wysyła Sygnał 1.
   - **Oczekiwany rezultat:** Ciężarówka natychmiast kończy załadunek i odjeżdża, mimo że posiada wolne miejsce. Na jej miejsce (jeśli dostępna) podstawia się kolejna ciężarówka, a proces ładowania z taśmy jest kontynuowany dla nowego pojazdu.

4. ✅ **Test ciągłości kolejki FIFO przy zmianie ciężarówki**
   - **Cel:** Weryfikacja, czy paczki nie giną i zachowują kolejność podczas momentu zmiany ciężarówek.
   - **Scenariusz:**
     1. Ciężarówka nr 1 zapełnia się całkowicie w momencie, gdy na taśmie wciąż znajdują się paczki.
     2. Ciężarówka nr 1 odjeżdża, podjeżdża ciężarówka nr 2.
   - **Oczekiwany rezultat:** Pierwsza paczka, która nie zmieściła się do ciężarówki nr 1, musi być pierwszą paczką załadowaną do ciężarówki nr 2. Żadna paczka nie może "zniknąć" w trakcie przełączania pojazdów, a kolejność "zdejmowania" z taśmy musi pozostać nienaruszona.
