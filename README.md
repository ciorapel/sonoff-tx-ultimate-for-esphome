### Sonoff TX Ultimate custom

Bazat pe https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome

Modificări aduse:
- scos complet nightlight automat folosind geolocație hardcodată
- introdus variabile pentru ascunderea din HA a butoanelor pentru întrerupătoarele cu mai puțin de 3 poziții (știu, se putea face mai frumos, dar ... merge și așa)
- reparat bug de toggle nightlight la fiecare restart
- ~~adăugat în tx_ultimate_touch.cpp filtrare pentru coduri UART invalide~~ revenit la cod inițial din cauza comportamentului suspect
- adăugat alte efecte pentru neopixel
- pus efectul de touch jos și indicatoarele de aprindere sus (întrerupătoarele montându-se la 50 cm de podea, nu se putea vedea ce lumină e aprinsă)
- modificat culori de indicare butoane, culori nightlight, intensități ale acestora etc.
- reparat stingerea părției cu efectul la 6 secunde de la apăsare
- tranziția între efectul de apăsare și nightlight se face acum fără stingerea ledurilor
- curățat cod de redundanțe
- etc.

Există și o variantă ESP-IDF funcțională dar în lucru, care momentan are bug cu mic flicker pe neopixel când întrerupătorul are mediaplayer activ și se acționează butoanele.

Pentru funcționalitatea pricipală: https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome

Conținut suficient pentru funcționare:
````yaml
substitutions:
  name: nume_intrerupator
  friendly_name: "nume_intrerupator"
  relay_count: "1" #numarul de poziții ale intrerupatorului
  relay_2_internal: "true" # dacă întrerupătorul are două poziții, valoarea trebuie să fie ”false”
  relay_3_internal: "true" # dacă întrerupătorul are trei poziții, valoarea trebuie să fie ”false”

packages:
  smarthomeyourself-crpl.tx-ultimate: github://ciorapel/sonoff-tx-ultimate-for-esphome/tx_ultimate.yaml@main
  
esphome:
  name: ${name}
  friendly_name: ${friendly_name}
````

