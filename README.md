# Sonoff TX Ultimate custom

Bazat pe <https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome>

Modificări aduse:

- scos complet nightlight automat folosind geolocație hardcodată
- introdus variabile pentru ascunderea din HA a butoanelor pentru întrerupătoarele cu mai puțin de 3 poziții (știu, se putea face mai frumos, dar ... merge și așa)
- reparat bug de toggle nightlight la fiecare restart
- adăugat alte efecte pentru neopixel
- pus efectul de touch jos și indicatoarele de aprindere sus
- modificat culori de indicare butoane, culori nightlight, intensități ale acestora etc.
- reparat stingerea partiției cu efectul de touch la 6 secunde de la apăsare
- tranziția între efectul de apăsare și nightlight se face acum fără stingerea ledurilor
- curățat cod de redundanțe
- adăugat modul decuplat
- adăugat mod DND, când efectele de pe banda de jos și vibrația nu mai sunt executate la activarea DND
- adăugat FAILSAFE la decoupled: la pierderea conexiunii cu HA înterupătorul devine cuplat automat la touch, chiar dacă inițial este setat în mod decuplat
- adăugată ascunderea zonelor de touchfield în funcție de numărul de poziții ale întrerupătorului
- nu mai este întrerupt efectul NeoPixel când se face update asupra unei entități (se apasă pe întrerupător fizic, se activează din HA, intervine nightlight etc.)
- etc.

Există și o variantă ESP-IDF funcțională dar în lucru, care momentan are bug cu mic flicker pe neopixel când întrerupătorul are mediaplayer activ și se acționează butoanele.

Pentru detalii privind codul care stă la baza acestui proiect: <https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome>

## Funcție decuplată

Dacă se dorește folosirea independentă a touch-ului față de releele interne, se va completa ”false” în dreptul poziției ce se dorește a fi folosită decuplat. În cazul acesta, în HA vor apărea două entități ale acelei poziții.
Exemplu: dacă se alege relay_1_coupled: "false", o să apară ”L1” și ”Dummy L1” în HA. ”L1” este în cazul ăsta releu intern care poate fi controlat doar din HA iar Dummy L1 este releu virtual ce este controlat prin apăsarea butonului de pe întrerupător.

Cazuri de utilizare a releului în mod decuplat:

Ai un alimentator de bandă LED care merge într-un controller WLED, și mai departe în banda LED.
Ai o lustră inteligentă care se poate controla radio / bluetooth / wireless / tuya etc.

Legi alimentatorul/lustra la un L pe care îl setezi decuplat, și din HA îl vei putea seta ON, setare ce va fi aplicată default, deci va fi alimentat permanent.
Automatizezi L-ul dummy din HA să trimită comenzi de ON/OFF către controllerul WLED/lustră, astfel se evită situația în care alimentarea este tăiată complet, iar acelui device îi va fi necesar ceva timp de la aprindere să se conecteze la HA.
Hint: Se pot folosi gesturile de swipe de pe întrerupător pentru controlarea intensității luminoase, și aici modul decuplat va fi foarte folositor menținând alimentarea permanentă pentru un răspuns instant.

Conținut suficient pentru funcționare:

````yaml
substitutions:
  name: nume_intrerupator
  friendly_name: "Nume intrerupator"
  relay_count: "3" #numarul de poziții ale intrerupatorului
  relay_2_internal: "false"
  relay_3_internal: "false"
  # Dacă întrerupătorul are o singură poziție, relay_2_internal și relay_3_internal se setează ambele "true"
  # Dacă întrerupătorul are două poziții, relay_2_internal se setează "false" și relay_3_internal "true"
  # Dacă întrerupătorul are trei poziții, se setează ambele "false"
                            
  relay_1_coupled: "true" # funcționare normală a L1 / "false" pentru modul decuplat
  relay_2_coupled: "true" # funcționare normală a L2 / "false" pentru modul decuplat
  relay_3_coupled: "true" # funcționare normală a L3 / "false" pentru modul decuplat

packages:
  smarthomeyourself-crpl.tx-ultimate: github://ciorapel/sonoff-tx-ultimate-for-esphome/tx_ultimate.yaml@main
````
