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
- ⚡ Adăugare sincronizare prin ESP-Now între întrerupătoare (*)
- 🧹 Filtrare completă a input-ului și detectare buton blocat  
- 🖐️ Funcția **stuck touch** pentru gesturi ne-liniare (swipe incomplet sau revenire la punctul inițial)  
- ⏱️ Generare automată a evenimentului de release după 500ms dacă nu este detectat  
- 🔄 Gestionarea corectă a long touch-urilor și readucerea releului la starea inițială
- ⚡ Adăugare posibilitate activare/dezactivare a releelor care să răspundă la multitouch

 (*) în cazul automatizărilor care fac toggle la releele sincronizate, trebuie adăugate toate releele în automatizare, nu doar unul. Ex. dacă o automatizare pornește L1 și L2 pe switch-ul master, datorită funcției de prevenire a buclei de sincronizare, pe swich-ul slave se va primi comanda de pornire doar L1; pentru a preveni această situație, în automatizare se vor adăuga ambele relee, atât cel master cât și cel slave.

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
  relay_count: "3" # numărul de poziții ale întrerupătorului
                            
  relay_1_coupled: "true" # true = normal, false = decuplat
  relay_2_coupled: "true"
  relay_3_coupled: "true"

  sync_enabled: "false"
  partner_mac: "30:C9:22:FD:E6:B0"

packages:
  smarthomeyourself-crpl.tx-ultimate: github://ciorapel/sonoff-tx-ultimate-for-esphome/tx_ultimate.yaml@main
```

## ⚙️ Configurări avansate

### Sincronizare ESP-NOW

```yaml
sync_enabled: "true"  # activează sincronizarea
partner_mac: "AA:BB:CC:DD:EE:FF"  # MAC-ul întrerupătorului pereche
```

### 🔄 Cum funcționează sincronizarea

- **Coupled true**: touch-ul controlează direct releul fizic care apare în HA și cele dummy nu apar
- **Coupled false**: touch-ul controlează doar dummy relay care acum sunt vizibile în HA, releul fizic rămâne independent și accesibil și el din HA
- **Internal true**: butonul nu apare în HA (pentru întrerupătoare cu mai puține poziții)

### 💡 Exemplu practic

**Întrerupător 2 poziții cu L2 decuplat:**

- `relay_2_coupled: "false"` → în HA vor apărea:
  - `L2` (releu fizic - controlat doar din HA)
  - `Dummy L2` (releu virtual - controlat prin touch)

## 📋 TODO

- [ ] dezactivare failback hotspot (dacă pică routerul principal se creează foarte multe hotspoturi, ceea ce generează interferențe puternice pe canalul WIFI și duce la pierderea a 75% din pachetele de sincronizare espnow); deocamdată rămâne activat din motive de debugging și update firmware OTA.
