# ⚡ Sonoff TX Ultimate Custom Firmware

Proiect personalizat pentru **Sonoff TX**, bazat pe [SmartHome-yourself/sonoff-tx-ultimate-for-esphome](https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome).

---

## 🔧 Modificări principale

- ⚡ Migrare completă de la **Arduino** la **ESP-IDF**  
- 🌙 Eliminarea nightlight-ului automat bazat pe geolocație hardcodată  
- 👀 Variabile pentru ascunderea butoanelor în Home Assistant pentru întrerupătoarele cu mai puțin de 3 poziții  
- 🐛 Corectarea bug-ului de toggle nightlight la fiecare restart  
- 🌈 Efecte suplimentare pentru **NeoPixel** și reorganizarea poziționării acestora  
- 🎨 Ajustarea culorilor și intensităților pentru butoane și nightlight  
- 🔄 Tranziție fluidă între efectul de apăsare și nightlight fără stingerea LED-urilor  
- 📴 Modul decuplat și modul **DND** (Do Not Disturb)  
- 🛡️ Failsafe: întrerupătorul devine automat cuplat dacă se pierde conexiunea cu Home Assistant  
- 🤹 Multitouch: gestionare simultană a tuturor butoanelor  
- ✋ Long touch: toggle nightlight  
- ⚙️ Optimizări C pentru apăsări scurte, prevenind pierderea evenimentelor de release  
- 🔕 Dezactivarea vibrației separat de DND  
- 💡 Indicatorii de aprindere au culoare și intensitate diferită în funcție de starea nightlight
- ⚡ Adăugare sincronizare prin ESP-Now între întrerupătoare.
- 🧹 Filtrare completă a input-ului și detectare buton blocat  
- 🖐️ Funcția **stuck touch** pentru gesturi ne-liniare (swipe incomplet sau revenire la punctul inițial)  
- ⏱️ Generare automată a evenimentului de release după 500ms dacă nu este detectat  
- 🔄 Gestionarea corectă a long touch-urilor și readucerea releului la starea inițială  

---

## 🛠️ Funcție decuplată (Decoupled Mode)

Permite folosirea touch-ului independent de releele interne.  

- Dacă se setează `false` pentru un releu (`relay_X_coupled: "false"`), în Home Assistant vor apărea două entități:  
  - **L(x)**: releul intern, controlat doar din HA  
  - **Dummy L(x)**: releu virtual, controlat prin touch-ul fizic al întrerupătorului  

### 💡 Exemple de utilizare

- Controlarea unei benzi LED prin WLED, păstrând alimentarea permanentă  
- Controlul unei lustre inteligente prin radio/Bluetooth/WiFi/Tuya  
- Swipe pe touch pentru controlul intensității luminoase fără a tăia alimentarea principală  

---

## ⚙️ Configurație minimă YAML

```yaml
substitutions:
  name: nume_intrerupator
  friendly_name: "Nume intrerupator"
  relay_count: "2" # numărul de poziții ale întrerupătorului
  relay_2_internal: "false" # setează ”true” dacă întrerupătorul are doar o poziție
  relay_3_internal: "true" # setează ”true” dacă întrerupătorul are doar două poziții
                            
  relay_1_coupled: "true" # true = normal, false = decuplat
  relay_2_coupled: "true"
  relay_3_coupled: "true"

  sync_enabled: "false"
  partner_mac: "30:C9:22:FD:F7:7C"

packages:
  smarthomeyourself-crpl.tx-ultimate: github://ciorapel/sonoff-tx-ultimate-for-esphome/tx_ultimate.yaml@main
```

## TODO

- dezactivare failback hotspot (dacă pică routerul principal se creează foarte multe hotspoturi, ceea ce generează interferențe puternice pe canalul WIFI și duce la pierderea a 75% din pachetele de sincronizare espnow); deocamdată rămâne activat din motive de debugging și update firmware OTA.
