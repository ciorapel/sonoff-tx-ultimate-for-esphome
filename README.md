### Sonoff TX Ultimate custom

Bazat pe https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome

Modificări aduse:
- scos complet nightlight automat folosind geolocație hardcodată
- introdus variabile pentru ascunderea din HA a butoanelor pentru întrerupătoarele cu mai puțin de 3 poziții (știu, se putea face mai frumos, dar ... merge și așa)
- reparat bug de toggle nightlight la fiecare restart

~~- adăugat în tx_ultimate_touch.cpp filtrare pentru coduri UART invalide~~ reverted
- adăugat alte efecte pentru neopixel
- pus efectul de touch jos și indicatoarele de aprindere sus (întrerupătoarele montându-se la 50 cm de podea, nu se putea vedea ce lumină e aprinsă)
- modificat culori de indicare butoane, culori nightlight, intensități ale acestora etc.
- reparat stingerea părției cu efectul la 6 secunde de la apăsare
- tranziția între efectul de apăsare și nightlight se face acum fără stingerea ledurilor
- curățat cod de redundanțe
- etc.

Există și o variantă ESP-IDF funcțională dar în lucru, care momentan are bug cu mic flicker pe neopixel când întrerupătorul are mediaplayer activ și se acționează butoanele.

Pentru funcționalitatea pricipală: https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome

===========================================


Based on https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome

Changes made:
- completely removed automatic nightlight using hardcoded geolocation
- introduced variables for hiding buttons for switches with less than 3 positions from HA (I know, it could have been done better, but... it works this way)
- fixed nightlight toggle bug on every restart

~~- added filtering for invalid UART codes in tx_ultimate_touch.cpp~~ reverted
- added other effects for neopixel
- put the touch effect down and the light indicators up (the switches are mounted 50 cm from the floor, so you couldn't see which light was on)
- changed button indicator colors, nightlight colors, their intensities, etc.
- fixed the effect turning off 6 seconds after pressing
- the transition between the press effect and nightlight is now done without turning off the LEDs
- cleaned up redundant code
- etc.

There is also a functional ESP-IDF version in progress, which currently has a bug with a slight flicker on the neopixel when the switch has the media player active and the buttons are pressed.

For the main functionality: https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome

Translated with DeepL.com (free version)
