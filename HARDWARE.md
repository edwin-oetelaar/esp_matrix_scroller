# Hardware-notities

## Hardwareversies

| | Vorige versie | Nieuwe versie |
|---|---|---|
| Ledstrip data | GPIO 33 | GPIO 33 |
| Ledstrip voeding | altijd aan | via MOSFET op **GPIO 21** |
| Status-LED (heartbeat) | GPIO 35 | GPIO 35 |

### GPIO 21: voeding ledstrip (nieuw)

Op de nieuwe hardware zit een MOSFET in de voedingslijn van de ledstrip,
aangestuurd door GPIO 21. **GPIO 21 moet hoog zijn**, anders krijgt de strip
geen voeding en blijft hij donker, ook al draait de firmware gewoon.

De firmware zet GPIO 21 hoog aan het begin van `LedTask`
(`LED_POWER_PIN` in `src/ledpanel.h`), voordat FastLED de strip aanstuurt.

Op de vorige hardware stuurt GPIO 21 de strip niet aan. Let op: op het
Adafruit Feather ESP32-S3-ontwerp (waar het board-profiel op gebaseerd is)
is GPIO 21 de voeding van de onboard NeoPixel/STEMMA QT. Hoog zetten is daar
onschuldig, maar controleer het als de vorige hardware iets anders op
GPIO 21 heeft aangesloten.
