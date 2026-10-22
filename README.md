# Matrix Scroller

Lichtkrant op basis van een ESP32-S3: een paneel van 128 × 8 WS2812-leds
waarop een zelfgekozen tekst scrolt. Tekst, kleuren en snelheid stel je in
vanaf je telefoon. Het board zet een eigen WiFi-netwerk op met een
captive portal, dus er is geen bestaand netwerk of internet nodig.

Gebruikt op **Dutch Design Week 2026** in Eindhoven.

## Gemaakt door

- **Kristiyan Angelov** ([meganukebmp](https://github.com/meganukebmp)): oorspronkelijk ontwerp en firmware
- **Edwin van den Oetelaar**: doorontwikkeling, stabiliteit en nieuwe hardwareversie

## Hardware

| Onderdeel | Details |
|---|---|
| Microcontroller | ESP32-S3, 8 MB flash, geen PSRAM (board-profiel `adafruit_feather_esp32s3_nopsram`) |
| Ledpaneel | 128 × 8 = 1024 WS2812-leds, serpentine per kolom, 5 V |
| Ledstrip data | GPIO 33 |
| Ledstrip voeding | MOSFET op GPIO 21 (alleen nieuwe hardware) |
| Status-LED | GPIO 35 (heartbeat) |

De verschillen tussen de hardwareversies staan in [HARDWARE.md](HARDWARE.md).

### Onderdelen kopen

Alle onderdelen zijn te koop bij [TinyTronics](https://www.tinytronics.nl/), en
het LED-controllerboard wordt ook door TinyTronics gemaakt. Zoek op de naam of
het artikelnummer (SKU).

Voor **één** lichtkrant heb je nodig:

| Aantal | Product | SKU | Prijs per stuk (incl. BTW) | Totaal (incl. BTW) |
|---:|---|---|---:|---:|
| 4 | WorldSemi WS2812B Digitale 5050 RGB LED - Matrix 32x8 - Flexibel | 002130 | € 22,00 | € 88,00 |
| 1 | Universele Digitale LED Strip Controller V2 - ESP32-S3 | 007920 | € 15,75 | € 15,75 |
| 1 | Mean Well Voeding - 5V 10A - Switching Power Supply - LRS-50-5 | 000699 | € 19,25 | € 19,25 |
| | | | **Totaal** | **€ 123,00** |

Vier panelen van 32×8 naast elkaar geven samen het display van 128×8.
Prijzen zoals besteld in oktober 2026 (€ 101,65 ex. BTW per set).

Met alle leds vol wit trekt het paneel ruim 20 A. De firmware begrenst het
vermogen op 10 A (`LED_MAX_MILLIAMPS` in `src/ledpanel.h`). Gebruik een 5 V-voeding
die dat aankan; de USB-poort van een laptop is daar niet geschikt voor.

## Gebruik

1. Zet het board aan en maak verbinding met het WiFi-netwerk
   **`Matrix XXXXXX`** (de laatste 6 tekens van het MAC-adres).
   Wachtwoord: `123456789`.
2. De instellingenpagina opent vanzelf. Gebeurt dat niet, ga dan naar
   <http://4.3.2.1>.
3. Vul tekst, voorgrondkleur, achtergrondkleur en snelheid in en verstuur.
   De instellingen worden in flash bewaard en blijven na een herstart staan.

Tekst die breder is dan het paneel scrolt; kortere tekst staat stil.

## Bouwen en flashen

Getest op Linux.

### Benodigdheden (eenmalig)

1. **Python 3.10 of nieuwer** en **git**.
2. **PlatformIO Core**:

   ```bash
   pipx install platformio
   ```

   Of gebruik de [PlatformIO-extensie voor VS Code](https://platformio.org/install/ide?install=vscode).
3. **Toegang tot de USB-seriële poort**: voeg jezelf toe aan de groep `dialout`
   en installeer de udev-regels van PlatformIO. Log daarna opnieuw in.

   ```bash
   sudo usermod -aG dialout $USER
   ```

   ```bash
   curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
   ```

De **eerste build** heeft internet nodig en duurt lang (±15 minuten op een
trage verbinding). PlatformIO downloadt dan het platform
([pioarduino](https://github.com/pioarduino/platform-espressif32)), de
Arduino-ESP32-core, de Xtensa-toolchain en de libraries, samen een paar honderd
MB. Platform en libraries staan vast op exacte versies in `platformio.ini`,
dus elke machine bouwt met precies dezelfde onderdelen.

### Offline bouwen

pioarduino controleert bij elke build of er internet is en gaat dan
Python-pakketten controleren en bijwerken. Zet dit uit, bijvoorbeeld in `~/.profile`:

```bash
export PLATFORMIO_OFFLINE=1
```

Na de eerste build gaat alles daarmee volledig offline (±9 s per build).

### Commando's

Firmware bouwen:

```bash
pio run
```

Firmware flashen:

```bash
pio run -t upload
```

Filesystem flashen (de webpagina `data/page.html` en de standaardconfig
`data/config`). **Let op:** dit overschrijft de op het board opgeslagen tekst
en instellingen.

```bash
pio run -t uploadfs
```

Seriële monitor (115200 baud):

```bash
pio device monitor
```

Bij een nieuw board flash je beide: eerst `upload`, daarna `uploadfs`.

### Board blijft na het flashen in download-modus

Soms start het board na een upload niet in de app maar blijft het in de
ROM-bootloader hangen (in `lsusb` zie je dan `303a:1001 Espressif USB JTAG`
in plaats van `239a:8113 Adafruit Feather ESP32-S3`). Herstart het dan via
een watchdog-reset, of trek de USB-kabel er even uit:

```bash
~/.platformio/penv/bin/esptool --port /dev/ttyACM0 --after watchdog-reset chip-id
```

## Projectstructuur

| Pad | Inhoud |
|---|---|
| `src/main.cpp` | Opstarten, heartbeat-LED |
| `src/ledpanel.*` | Led-task: tekst renderen, scrollen, config in flash |
| `src/networking.*` | WiFi-access point, DNS (captive portal) en webserver |
| `src/font*.h` | Lettertypes |
| `data/` | Bestanden voor het LittleFS-filesystem |
| `partitions.csv` | Partitietabel (2× OTA van 2 MB, LittleFS, coredump) |
| `HARDWARE.md` | Verschillen tussen hardwareversies |

## Technische notities

- De leds worden aangestuurd met FastLED via de RMT-peripheral. Een frame naar
  1024 leds sturen duurt ±31 ms. FastLED 3.10.3 overschrijft zijn
  verzendbuffer al voordat het vorige frame klaar is, daarom stuurt de firmware
  nooit sneller dan één frame per 37 ms. Anders krijg je tearing. De
  snelheidsschuif (10–255) loopt van 255 ms tot 37 ms per scrollstap.

## Licentie

[MIT](LICENSE) © 2025-2026 Kristiyan Angelov en Edwin van den Oetelaar
