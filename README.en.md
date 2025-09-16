#### Choose language:
[![en](https://img.shields.io/badge/lang-en-red.svg)](https://github.com/ciorapel/sonoff-tx-ultimate-for-esphome/blob/main/README.en.md) 
[![ro](https://img.shields.io/badge/lang-ro-yellow.svg)](https://github.com/ciorapel/sonoff-tx-ultimate-for-esphome/blob/main/README.md)

# ⚡ Sonoff TX Ultimate Custom Firmware

Custom project for **Sonoff TX**, based on [SmartHome-yourself/sonoff-tx-ultimate-for-esphome](https://github.com/SmartHome-yourself/sonoff-tx-ultimate-for-esphome).

---

## 🔧 Main changes

- ⚡ Complete migration from **Arduino** to **ESP-IDF**
- 🌙 Removal of automatic nightlight based on hardcoded geolocation
- 👀 Auto-hide entities in Home Assistant for switches with less than 3 buttons  
- 🐛 Fixed the toggle nightlight bug on every restart  
- 🌈 Additional effects for **NeoPixel** and reorganization of their positioning  
- 🎨 Adjustment of colors and intensities for buttons and nightlight  
- 🔄 Smooth transition between the press effect and nightlight without turning off the LEDs  
- 📴 Decoupled mode and **DND** mode (Do Not Disturb)  
- 🛡️ Failsafe: the switch automatically becomes coupled if the connection to Home Assistant is lost  
- 🤹 Multitouch: simultaneous management of all buttons  
- ✋ Long touch: toggle nightlight  
- ⚙️ C optimizations for short presses, preventing loss of release events  
- 🔕 Vibration deactivation separate from DND  
- 💡 The light indicators have different colors and intensities depending on the nightlight status
- ⚡ Add ESP-Now synchronization between switches
- 🧹 Full input filtering and stuck button detection
- 🖐️ **Stuck touch** function for non-linear gestures (incomplete swipe or return to the starting point)  
- ⏱️ Automatic generation of the release event after 500ms if not detected  
- 🔄 Correct management of long touches and return of the relay to its initial state
- ⚡ Add option to enable/disable relays that respond to multitouch

---

## 🛠️ Decoupled Mode

Allows the use of touch independently of internal relays.

- If `false` is set for a relay (`relay_X_coupled: "false"`), two entities will appear in Home Assistant:  
  - **L(x)**: internal relay, controlled only from HA
  - **Dummy L(x)**: virtual relay, controlled by the physical touch of the switch

### 💡 Examples of use

- Controlling an LED strip via WLED, keeping the power on permanently
- Controlling a smart chandelier via radio/Bluetooth/WiFi/Tuya
- Swipe on the touch to control the light intensity without cutting the main power supply  

---

## ⚙️ Minimum YAML configuration

```yaml
substitutions:
  name: nume_intrerupator
  friendly_name: "Nume intrerupator"
  relay_count: "3" # number of switch buttons
                            
  relay_1_coupled: "true" # true = normal, false = decoupled
  relay_2_coupled: "true"
  relay_3_coupled: "true"

  sync_enabled: "false"
  partner_mac: "30:C9:22:FD:E6:B0"

packages:
  smarthomeyourself-crpl.tx-ultimate: github://ciorapel/sonoff-tx-ultimate-for-esphome/tx_ultimate.yaml@main
```

## ⚙️ Advanced settings

### ESP-NOW synchronization

```yaml
sync_enabled: "true"  # enable synchronization
partner_mac: "AA:BB:CC:DD:EE:FF"  # MAC address of the paired switch
```


### 🔄 How coupled mode works

- **Coupled true**: the touch directly controls the physical relay that appears in HA, and the dummy relays do not appear
- **Coupled false**: the touch controls only the dummy relays that are now visible in HA, the physical relay remains independent and also accessible from HA

### 💡 Practical example

**2-position switch with L2 disconnected:**

- `relay_2_coupled: "false"` → the following will appear in HA:
  - `L2` (physical relay - controlled only from HA)
  - `Dummy L2` (virtual relay - controlled by touch)

## 📋 TODO

- [ ] disable hotspot failback (if the main router fails, many hotspots are created, which generates strong interference on the WIFI channel and leads to the loss of 75% of the espnow synchronization packets); for now, it remains enabled for debugging and OTA firmware update reasons.