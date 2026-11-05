# WUP-028 ESP32-S2 + UART z projektu DS3 v2 (eksperyment)

Cel: zachowac dzialajaca warstwe USB WUP-028 z `wup028_s2_diagnostic.zip`, a zamiast automatycznego A odczytywac ramki UART z projektu DS3 v2.

## Co zostało potwierdzone
- W dostarczonym ZIP DS3 v2 jest odbiornik ESP32-S2 na GPIO16, 115200 8N1.
- Ramka: A5 5A, numer, przyciski uint16 little-endian, hat, LX/LY/RX/RY signed int8, LT/RT, flaga polaczenia, rezerwa, CRC-8. Lacznie 20 bajtow.
- Test lokalny mapowania ramki na raport GC przeszedl. To NIE jest test sprzetowy ani kompilacja ESP-IDF.
- Warstwa USB pochodzi z firmware USB-only, ktory uzytkownik testowal w Twin Snakes.

## Pliki do dodania do repozytorium
- `diagnostics/s2_uart_wup028/main.c`
- `diagnostics/s2_uart_wup028/bridge.c`
- `diagnostics/s2_uart_wup028/pad_packet.h`
- `diagnostics/s2_uart_wup028/pad_gc.h`
- `.github/workflows/uart-wup028-s2.yml`

Workflow kompiluje na GitHub Actions: `Build ESP32-S2 UART to WUP-028`. Nie zmienia plikow zrodlowych istniejacego projektu w repozytorium (kopiuje je jedynie w runnerze).

## Przewody
- LOLIN TX z pierwotnego firmware UART -> ESP32-S2 GPIO16 (RX)
- LOLIN GND -> ESP32-S2 GND
- Zasilanie obydwu plytek osobno po USB. Nie lacz 5V ani 3V3. Poziom UART 3.3V.
- **UWAGA**: przeslany ZIP nie zawiera plikow zrodlowych LOLIN. Trzeba sprawdzic rzeczywisty pin TX oraz czy uzyty firmware nadaje opisane 20-bajtowe pakiety. Nie wystarczy zwykle BlueRetro Wii-Ext/GC.

## Zachowanie
- Port 1 raportowany stale jako przewodowy, tak jak we wczesniejszym USB-only.
- Przycisk A nie jest generowany automatycznie.
- PAD_A (bit 0) -> GC A; START -> GC Start; dpad, osie, L/R/Z i triggery sa mapowane.
- Gdy UART zamilknie >120 ms, stan przechodzi na neutralny (bez wcisnietych przyciskow).
- Porty 2-4 puste. Bez wibracji.

## Wgrywanie
Pobierz artefakt `WUP028-S2-UART-Bridge`. Sprawdz adresy we `flasher_args.json`. Zachowaj kopie dzialajacej wersji USB-only; nie nadpisuj bootloadera na slepo.

## Kolejnosc testow
1. Najpierw Windows/Dolphin: adapter jest wykrywany, przyciski DualSense widoczne na porcie 1.
2. Potem Wii U / Twin Snakes.
3. Jezeli Dolphin wykrywa adapter, ale przyciski nie reaguja, sprawdz najpierw, czy LOLIN nadaje 20-bajtowe ramki UART. Ten ZIP nie dowodzi działania nadajnika.

## Ograniczenia
Nie wykonywano kompilacji ESP-IDF ani testu na prawdziwym ESP32-S2. Test C sprawdza jedynie dekodowanie raportu. Nie jest to gotowy, potwierdzony firmware.
