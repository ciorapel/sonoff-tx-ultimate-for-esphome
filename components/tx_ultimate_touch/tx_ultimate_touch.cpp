#include "esphome/core/log.h"
#include "tx_ultimate_touch.h"

namespace esphome
{
    namespace tx_ultimate_touch
    {
        static const char *TAG = "tx_ultimate_touch";

        void TxUltimateTouch::setup()
        {
            ESP_LOGI("log", "%s", "Tx Ultimate Touch is initialized");
        }

        void TxUltimateTouch::loop()
        {
            static int bytes[15] = {};
            static int i = 0;
            static unsigned long last_activity = 0;
            static bool packet_started = false;
            
            bool found = false;
            int byte = -1;
            
            unsigned long current_time = millis();

            while (this->available())
            {
                byte = this->read();
                last_activity = current_time;
                
                // Detectează începutul unui pachet nou
                if (byte == 170) // 0xAA
                {
                    // Dacă aveam deja un pachet în curs, procesează-l
                    if (packet_started && i > 4)
                    {
                        handle_touch(bytes);
                    }
                    
                    // Resetează pentru noul pachet
                    memset(bytes, 0, sizeof(bytes));
                    i = 0;
                    packet_started = true;
                }

                if (packet_started && i < 15)
                {
                    bytes[i] = byte;
                    i++;
                    
                    // Verifică dacă avem un pachet complet (15 bytes)
                    if (i >= 15)
                    {
                        handle_touch(bytes);
                        packet_started = false;
                        i = 0;
                        found = true;
                    }
                }

                if (byte != 0)
                {
                    found = true;
                }
            }

            // Timeout pentru pachete incomplete - dacă nu am primit date de 100ms
            // și avem un pachet parțial, consideră-l invalid
            if (packet_started && (current_time - last_activity > 100) && i > 0)
            {
                ESP_LOGW(TAG, "Packet timeout - discarding incomplete packet (i=%d)", i);
                packet_started = false;
                i = 0;
                memset(bytes, 0, sizeof(bytes));
            }

            // Procesează ultimul pachet doar dacă este complet și valid
            if (found && packet_started && i >= 4)
            {
                // Verifică dacă pare a fi un pachet valid înainte de procesare
                if (bytes[0] == 170 && bytes[1] == 85 && bytes[2] == 1 && bytes[3] == 2)
                {
                    handle_touch(bytes);
                    packet_started = false;
                    i = 0;
                }
            }
        }

        void TxUltimateTouch::handle_touch(int bytes[])
        {
            ESP_LOGV("UART-Log", "------------");
            for (int i = 0; i < 15; i++)
            {
                ESP_LOGV("UART-Log", "%i", bytes[i]);
            }

            if (is_valid_data(bytes))
            {
                send_touch_(get_touch_point(bytes));
            }
            else
            {
                ESP_LOGW(TAG, "Invalid touch data received");
            }
        }

        void TxUltimateTouch::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Tx Ultimate Touch");
        }

        void TxUltimateTouch::send_touch_(TouchPoint tp)
        {
            // Adaugă debouncing pentru a evita evenimente duplicate rapide
            static unsigned long last_event_time = 0;
            static int last_event_state = -1;
            static int last_event_x = -1;
            
            unsigned long current_time = millis();
            
            // Ignore duplicate events within 50ms
            if (current_time - last_event_time < 50 && 
                tp.state == last_event_state && 
                tp.x == last_event_x)
            {
                ESP_LOGV(TAG, "Debouncing duplicate event");
                return;
            }
            
            last_event_time = current_time;
            last_event_state = tp.state;
            last_event_x = tp.x;

            switch (tp.state)
            {
            case TOUCH_STATE_RELEASE:
                if (tp.x >= 17)
                {
                    tp.x = tp.x - 16;
                    ESP_LOGD(TAG, "Long Press Release (x=%d)", tp.x);
                    this->long_touch_release_trigger_.trigger(tp);
                }
                else
                {
                    ESP_LOGD(TAG, "Release (x=%d)", tp.x);
                    this->release_trigger_.trigger(tp);
                }
                break;

            case TOUCH_STATE_PRESS:
                ESP_LOGD(TAG, "Press (x=%d)", tp.x);
                this->touch_trigger_.trigger(tp);
                break;

            case TOUCH_STATE_SWIPE_LEFT:
                ESP_LOGD(TAG, "Swipe Left (x=%d)", tp.x);
                this->swipe_trigger_left_.trigger(tp);
                break;

            case TOUCH_STATE_SWIPE_RIGHT:
                ESP_LOGD(TAG, "Swipe Right (x=%d)", tp.x);
                this->swipe_trigger_right_.trigger(tp);
                break;

            case TOUCH_STATE_ALL_FIELDS:
                ESP_LOGD(TAG, "Full Touch Release");
                this->full_touch_release_trigger_.trigger(tp);
                break;

            default:
                ESP_LOGW(TAG, "Unknown touch state: %d", tp.state);
                break;
            }
        }

        bool TxUltimateTouch::is_valid_data(int bytes[])
        {
            // Verifică header-ul pachetului
            if (!(bytes[0] == 170 && bytes[1] == 85 && bytes[2] == 1 && bytes[3] == 2))
            {
                ESP_LOGV(TAG, "Invalid packet header: %d %d %d %d", bytes[0], bytes[1], bytes[2], bytes[3]);
                return false;
            }

            int state = get_touch_state(bytes);
            if (state != TOUCH_STATE_PRESS &&
                state != TOUCH_STATE_RELEASE &&
                state != TOUCH_STATE_SWIPE_LEFT &&
                state != TOUCH_STATE_SWIPE_RIGHT &&
                state != TOUCH_STATE_ALL_FIELDS)
            {
                ESP_LOGV(TAG, "Invalid touch state: %d", state);
                return false;
            }

            // Pentru starea ALL_FIELDS, poziția poate fi diferită
            if (bytes[6] < 0 && state != TOUCH_STATE_ALL_FIELDS)
            {
                ESP_LOGV(TAG, "Invalid position data: %d for state %d", bytes[6], state);
                return false;
            }

            return true;
        }

        int TxUltimateTouch::get_x_touch_position(int bytes[])
        {
            int state = bytes[4];
            switch (state)
            {
            case TOUCH_STATE_RELEASE:
                return bytes[5];
                break;

            case TOUCH_STATE_ALL_FIELDS:
                return bytes[5];
                break;

            case TOUCH_STATE_SWIPE_LEFT:
                return bytes[5];
                break;

            case TOUCH_STATE_SWIPE_RIGHT:
                return bytes[5];
                break;

            default:
                return bytes[6];
                break;
            }
        }

        int TxUltimateTouch::get_touch_state(int bytes[])
        {
            int state = bytes[4];

            if (state == TOUCH_STATE_PRESS && bytes[5] != 0)
            {
                state = TOUCH_STATE_RELEASE;
            }

            if (state == TOUCH_STATE_RELEASE && bytes[5] == TOUCH_STATE_ALL_FIELDS)
            {
                state = TOUCH_STATE_ALL_FIELDS;
            }

            if (state == TOUCH_STATE_SWIPE)
            {
                if (bytes[5] == TOUCH_STATE_SWIPE_RIGHT)
                {
                    state = TOUCH_STATE_SWIPE_RIGHT;
                }
                else if (bytes[5] == TOUCH_STATE_SWIPE_LEFT)
                {
                    state = TOUCH_STATE_SWIPE_LEFT;
                }
            }

            return state;
        }

        TouchPoint TxUltimateTouch::get_touch_point(int bytes[])
        {
            TouchPoint tp;

            tp.x = get_x_touch_position(bytes);
            tp.state = get_touch_state(bytes);

            return tp;
        }

    } // namespace tx_ultimate_touch
} // namespace esphome