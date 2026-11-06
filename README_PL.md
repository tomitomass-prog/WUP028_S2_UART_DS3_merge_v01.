# ESP32-S2 WUP-028 + LOLIN UART (GOLD v0.2)

## Cel
Ta wersja zachowuje obsługę USB z kodu diagnostycznego `diagnostics/s2_usb_only` (ten projekt generował A co 4 s i był bazą udanych testów na Wii U). Odbiór UART wydzielono do nowego pliku, żeby można było łatwo porównać działanie dwóch obrazów.

**WAŻNE:** zachowany plik `usb-gcca(2).bin` to potwierdzona na Wii U wersja referencyjna. Nie jesteśmy w stanie dowieść, że źródła diagnostyczne w tej paczce są dokładnie tym samym commitem, z którego powstał ten BIN. Dopóki nie przeprowadzimy testów, nie nazywaj nowego obrazu sprawdzonym firmware Wii U.

## Zawartość
- `diagnostics/s2_gold_uart/main.c`: źródłowa konfiguracja TinyUSB i timera 1 ms. UART uruchamia się dopiero po USB i tylko w wariancie UART.
- `diagnostics/s2_gold_uart/bridge.c`: oryginalna obsługa USB, ACK, origin i portu GC. Jedyną zmianą w raportach jest przełączanie źródła danych (automatyczny A lub UART).
- `diagnostics/s2_gold_uart/uart_receiver.c`: nowy odbiornik 115200 8N1, 20-bajtowe ramki A5 5A + CRC8; brak `ESP_ERROR_CHECK` w inicjalizacji UART, aby ewentualny błąd UART nie restartował urządzenia USB.
- `diagnostics/s2_gold_uart/pad_packet.h`, `pad_gc.h`: format nadajnika LOLIN i mapowanie do portu GC.
- `tests/test_pad_gc.c`: natywny test CRC, A, START, L i D-pada.
- `.github/workflows/build-s2-gold-uart.yml`: kompiluje DWA warianty w Github Actions, z bazą źródeł `usb-gamecube-controller-adapter` przypiętą do tagu `v0.9.0`.

## GitHub Actions
1. Utwórz osobne repozytorium, ewentualnie wgraj całą zawartość paczki do istniejącego repozytorium (katalog `.github` musi zostać zachowany). Nie kopiuj podwójnego katalogu nadrzędnego.
2. Otwórz `Actions -> WUP-028 ESP32-S2 GOLD baseline and UART -> Run workflow`.
3. Powinny powstać dwa artefakty:
   - `WUP028-S2-usb-only-control`: wersja bez UART, nadal wysyłająca A co około 4 sekundy.
   - `WUP028-S2-uart`: wersja z rzeczywistymi przyciskami z LOLIN; bez napływających ramek przyciski są neutralne.
4. Jeśli workflow nie przejdzie, prześlij PEŁNY tekst pierwszego czerwonego kroku. W paczce nie ma gotowych obrazów .bin, ponieważ nie uruchamialiśmy kompilacji ESP-IDF 5.2.

## Bezpieczna kolejność testów
1. ZARCHIWIZUJ `usb-gcca(2).bin` i nie nadpisuj pliku źródłowego. Nie kasuj całego Flash przed testem.
2. Najpierw wgraj `WUP028-S2-usb-only-control` według `flasher_args.json` danego artefaktu i sprawdź STABILNE wykrycie przez Dolphin. To test odtwarzalności źródeł i toolchainu. Nie zakładaj, że ten obraz jest identyczny z oryginalnym BIN.
3. Dopiero po stabilnym wykryciu wgraj `WUP028-S2-uart`, nadal z LOLIN odłączonym. W Dolphin WUP-028 powinien być stabilnie widoczny, ale bez automatycznego A.
4. Jeśli UART bez LOLIN powoduje miganie w Dolphin, NIE podłączaj LOLIN do Wii U. Wróć do zachowanego `usb-gcca(2).bin` i przekaż wynik. To wskazuje na regresję w nowej aplikacji.
5. Jeśli USB jest stabilne, odłącz zasilanie obu płytek i połącz LOLIN GPIO17 (TX) -> ESP32-S2 GPIO16 (RX), oraz GND -> GND. Żadnego 5 V/3V3 pomiędzy zasilanymi osobno płytkami.
6. LOLIN musi mieć firmware Bluepad32 z programu `bluepad32_sender.cpp`, wysyłający co 10 ms ramki UART 115200 baud, nie standardowy BlueRetro Wii-Ext ani GC.
7. Włącz DualSense, upewnij się, że sparował się z LOLIN, i w Dolphin sprawdź port 1, przyciski i analogi. Potem dopiero Wii U.

## Czym ten projekt NIE jest
- Nie wykorzystuje Joybus, I2C ani emulacji DualShock 3 na USB.
- Nie zawiera firmware dla LOLIN i nie zmienia go. Wcześniej przesłany obraz `firmware(4).bin` miał komunikat `LOLIN32 Lite: DualSense -> UART ready`, ale samo istnienie komunikatu nie stanowi dowodu identycznego formatu bajtów; test na sprzęcie jest konieczny.
- Nie został jeszcze skompilowany w GitHub Actions ani przetestowany na sprzęcie.

## Mapowanie
- PAD_A -> GC A, PAD_B -> GC B, PAD_X -> GC X, PAD_Y -> GC Y.
- PAD_START -> GC START, PAD_L / PAD_R -> GC L / R, PAD_ZL/PAD_ZR -> GC Z.
- D-pad, dwie gałki, LT/RT -> osie i analogowe triggery GC.
- Pole `connected=0` albo cisza UART >150 ms -> neutralne przyciski (pad nadal zgłaszany na porcie 1, jak w oryginalnym teście USB).

## Gdy kontroler działa odwrotnie
Orientacja osi Y i rozkład przycisków DualSense / Bluepad32 mogą wymagać późniejszego mapowania. NIE modyfikuj na razie deskryptorów USB WUP-028.
