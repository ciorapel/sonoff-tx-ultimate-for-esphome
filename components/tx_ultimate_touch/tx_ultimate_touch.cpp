#include "esphome/core/log.h"
#include "tx_ultimate_touch.h"
#include <cinttypes>

namespace esphome {
    namespace tx_ultimate_touch {

        void TxUltimateTouch::setup() {
            ESP_LOGI(TAG, "TX Ultimate Touch is initialized");
            this->touch_state_ = TOUCH_IDLE;
        }

        void TxUltimateTouch::loop() {
            // Process UART packets first
            this->process_uart_packets();
            
            // Handle state machine with single timer
            this->handle_touch_state_machine();
        }

        void TxUltimateTouch::process_uart_packets() {
            static std::array<uint8_t, UART_BUFFER_SIZE> bytes{};
            static int i = 0;
            static bool found = false;
            
            while (this->available()) {
                uint8_t byte = this->read();
                
                if (byte == HEADER_BYTE_1) {
                    if (found && i >= 6) {
                        this->handle_touch(bytes);
                    }
                    bytes.fill(0);
                    i = 0;
                    found = false;
                }
                
                if (i < UART_BUFFER_SIZE) {
                    bytes[i] = byte;
                    i++;
                }
                
                if (byte != 0x00) {
                    found = true;
                }
            }
            
            if (found && i >= 6) {
                this->handle_touch(bytes);
                found = false;
            }
        }

        void TxUltimateTouch::handle_touch_state_machine() {
            unsigned long current_time = millis();
            unsigned long elapsed = current_time - this->state_start_time_;
            
            switch (this->touch_state_) {
                case TOUCH_IDLE:
                    // Nothing to do in idle state
                    break;
                    
                case TOUCH_PRESSED:
                    if (elapsed >= AUTO_RELEASE_TIMEOUT) {
                        ESP_LOGW(TAG, "Auto-releasing stuck touch after %lums", elapsed);
                        this->force_release();
                        this->transition_to_state(TOUCH_AUTO_RELEASED);
                    }
                    break;
                    
                case TOUCH_AUTO_RELEASED:
                    if (elapsed >= LONG_PRESS_TIMEOUT) {
                        ESP_LOGD(TAG, "Touch state timeout, returning to idle");
                        this->transition_to_state(TOUCH_IDLE);
                    }
                    break;
            }
        }

        void TxUltimateTouch::transition_to_state(TouchStateMachine new_state) {
            ESP_LOGV(TAG, "State transition: %d -> %d", this->touch_state_, new_state);
            this->touch_state_ = new_state;
            this->state_start_time_ = millis();
        }

        void TxUltimateTouch::force_release() {
            if (this->last_press_x_ == INVALID_VALUE) return;
            
            TouchPoint release_tp;
            release_tp.x = this->last_press_x_;
            release_tp.state = TOUCH_STATE_RELEASE;
            release_tp.state_str = this->get_state_string(release_tp.state);

            ESP_LOGD(TAG, "Forced release at position %u", release_tp.x);
            this->release_trigger_.trigger(release_tp);
            this->touch_event_trigger_.trigger(release_tp);
        }

        void TxUltimateTouch::handle_touch(const std::array<uint8_t, UART_BUFFER_SIZE> &bytes) {
            ESP_LOGV(TAG, "Raw packet data:");
            
            int packet_len = UART_BUFFER_SIZE;
            for (int j = UART_BUFFER_SIZE - 1; j >= 0; j--) {
                if (bytes[j] != 0) {
                    packet_len = j + 1;
                    break;
                }
            }
            
            for (int i = 0; i < packet_len; i++) {
                ESP_LOGV(TAG, "  [%d]: %u", i, bytes[i]);
            }

            if (this->is_valid_data(bytes)) {
                this->send_touch_(this->get_touch_point(bytes));
            } else {
                ESP_LOGW(TAG, "Invalid touch data received");
            }
        }

        void TxUltimateTouch::dump_config() {
            ESP_LOGCONFIG(TAG, "TX Ultimate Touch");
            ESP_LOGCONFIG(TAG, "  Max position: %u", TOUCH_MAX_POSITION);
            ESP_LOGCONFIG(TAG, "  Auto-release timeout: %u ms", AUTO_RELEASE_TIMEOUT);
            ESP_LOGCONFIG(TAG, "  Long press timeout: %u ms", LONG_PRESS_TIMEOUT);
            ESP_LOGCONFIG(TAG, "  Long press threshold: %u ms", LONG_PRESS_THRESHOLD);
            ESP_LOGCONFIG(TAG, "  Debounce time: %u ms", DEBOUNCE_TIME_MS);
        }

        void TxUltimateTouch::send_touch_(TouchPoint tp) {
            if (tp.x == INVALID_VALUE || tp.state == INVALID_VALUE) {
                ESP_LOGW(TAG, "Ignoring invalid touch point (x=%u, state=%u)", tp.x, tp.state);
                return;
            }
            
            // Enhanced debouncing
            static unsigned long last_event_time = 0;
            static uint8_t last_event_state = INVALID_VALUE;
            static uint8_t last_event_x = INVALID_VALUE;
            
            unsigned long current_time = millis();
            
            tp.state_str = this->get_state_string(tp.state);
            
            if (current_time - last_event_time < DEBOUNCE_TIME_MS && 
                tp.state == last_event_state && 
                tp.x == last_event_x) {
                ESP_LOGV(TAG, "Debouncing duplicate event");
                return;
            }
            
            last_event_time = current_time;
            last_event_state = tp.state;
            last_event_x = tp.x;

            // Always trigger universal event first
            this->touch_event_trigger_.trigger(tp);

            // Handle state machine and specific events
            switch (tp.state) {
                case TOUCH_STATE_PRESS:
                    if (tp.x <= TOUCH_MAX_POSITION) {
                        ESP_LOGD(TAG, "Press at position %u", tp.x);
                        this->touch_trigger_.trigger(tp);
                        
                        // Update state machine
                        this->last_press_x_ = tp.x;
                        this->transition_to_state(TOUCH_PRESSED);
                    } else {
                        ESP_LOGW(TAG, "Invalid press position: %u", tp.x);
                    }
                    break;

                case TOUCH_STATE_RELEASE:
                    this->handle_release_event(tp, current_time);
                    break;

                case TOUCH_STATE_SWIPE_LEFT:
                    ESP_LOGD(TAG, "Swipe Left at position %u", tp.x);
                    this->swipe_trigger_left_.trigger(tp);
                    this->transition_to_state(TOUCH_IDLE);
                    break;

                case TOUCH_STATE_SWIPE_RIGHT:
                    ESP_LOGD(TAG, "Swipe Right at position %u", tp.x);
                    this->swipe_trigger_right_.trigger(tp);
                    this->transition_to_state(TOUCH_IDLE);
                    break;

                case TOUCH_STATE_ALL_FIELDS:
                    ESP_LOGD(TAG, "Multi Touch Release");
                    this->full_touch_release_trigger_.trigger(tp);
                    this->multi_touch_release_trigger_.trigger(tp);
                    this->transition_to_state(TOUCH_IDLE);
                    break;

                default:
                    ESP_LOGW(TAG, "Unknown touch state: %u (%s)", tp.state, tp.state_str.c_str());
                    break;
            }
        }

        void TxUltimateTouch::handle_release_event(TouchPoint tp, unsigned long current_time) {
            // Check for long press if we were in auto-released state
            if (this->touch_state_ == TOUCH_AUTO_RELEASED) {
                unsigned long total_press_time = current_time - this->state_start_time_ + AUTO_RELEASE_TIMEOUT;
                
                if (total_press_time >= LONG_PRESS_THRESHOLD) {
                    ESP_LOGI(TAG, "Long press detected! Duration: %lums at position %u", total_press_time, this->last_press_x_);
                    
                    // Create long press event using original position
                    TouchPoint long_press_tp = tp;
                    long_press_tp.x = this->last_press_x_;
                    long_press_tp.state_str = "LONG_PRESS_RELEASE";
                    
                    this->long_touch_release_trigger_.trigger(long_press_tp);
                    
                    // Simulate touch & release sequence
                    this->simulate_touch_sequence(long_press_tp.x);
                    this->transition_to_state(TOUCH_IDLE);
                    return;
                }
            }
            
            // Handle hardware long press (with offset detection)
            if (tp.x >= (LONG_PRESS_OFFSET + 1) && tp.x < (LONG_PRESS_OFFSET + TOUCH_MAX_POSITION + LONG_PRESS_OFFSET)) {
                uint8_t adjusted_x = tp.x - LONG_PRESS_OFFSET;
                tp.x = adjusted_x;
                ESP_LOGD(TAG, "Hardware Long Press Release at position %u", tp.x);
                this->long_touch_release_trigger_.trigger(tp);
            } else if (tp.x <= TOUCH_MAX_POSITION) {
                ESP_LOGD(TAG, "Release at position %u", tp.x);
                this->release_trigger_.trigger(tp);
            } else {
                ESP_LOGW(TAG, "Invalid release position: %u", tp.x);
            }
            
            this->transition_to_state(TOUCH_IDLE);
        }

        void TxUltimateTouch::simulate_touch_sequence(uint8_t position) {
            ESP_LOGD(TAG, "Simulating touch & release sequence at position %u", position);
            
            TouchPoint sim_press;
            sim_press.x = position;
            sim_press.state = TOUCH_STATE_PRESS;
            sim_press.state_str = "SIMULATED_PRESS";
            
            TouchPoint sim_release = sim_press;
            sim_release.state = TOUCH_STATE_RELEASE;
            sim_release.state_str = "SIMULATED_RELEASE";
            
            // Send simulated events
            this->touch_trigger_.trigger(sim_press);
            this->touch_event_trigger_.trigger(sim_press);
            this->release_trigger_.trigger(sim_release);
            this->touch_event_trigger_.trigger(sim_release);
        }

        bool TxUltimateTouch::is_valid_data(const std::array<uint8_t, UART_BUFFER_SIZE> &bytes) const {
            // Check packet header
            if (bytes[0] != HEADER_BYTE_1 || 
                bytes[1] != HEADER_BYTE_2 || 
                bytes[2] != PACKET_VERSION || 
                bytes[3] != PACKET_OPCODE) {
                ESP_LOGV(TAG, "Invalid packet header: %u %u %u %u", 
                        bytes[0], bytes[1], bytes[2], bytes[3]);
                return false;
            }

            uint8_t state = this->get_touch_state(bytes);
            if (state != TOUCH_STATE_PRESS &&
                state != TOUCH_STATE_RELEASE &&
                state != TOUCH_STATE_SWIPE_LEFT &&
                state != TOUCH_STATE_SWIPE_RIGHT &&
                state != TOUCH_STATE_ALL_FIELDS) {
                ESP_LOGV(TAG, "Invalid touch state: %u", state);
                return false;
            }

            // Position validation - multi-touch events may have different position encoding
            if (state != TOUCH_STATE_ALL_FIELDS) {
                uint8_t x = this->get_x_touch_position(bytes);
                // Allow positions up to LONG_PRESS_OFFSET + TOUCH_MAX_POSITION for hardware long presses
                if (x > (LONG_PRESS_OFFSET + TOUCH_MAX_POSITION)) {
                    ESP_LOGV(TAG, "Invalid position: %u for state %u", x, state);
                    return false;
                }
            }

            return true;
        }

        uint8_t TxUltimateTouch::get_x_touch_position(const std::array<uint8_t, UART_BUFFER_SIZE> &bytes) const {
            // Enhanced position detection from multiple implementations
            switch (bytes[4]) {
                case TOUCH_STATE_RELEASE:
                case TOUCH_STATE_ALL_FIELDS:
                    return bytes[5];

                case TOUCH_STATE_SWIPE_LEFT:
                case TOUCH_STATE_SWIPE_RIGHT:
                    // Advanced swipe position detection
                    switch (bytes[5]) {
                        case 12: // Scan right to left
                            for (uint8_t i = TOUCH_MAX_POSITION; i > 0; i--) {
                                if (i > 8
                                    ? bytes[6] & (1 << (i - 9))
                                    : bytes[7] & (1 << (i - 1))) {
                                    return i;
                                }
                            }
                            break;
                        case 13: // Scan left to right
                            for (uint8_t i = 1; i <= TOUCH_MAX_POSITION; i++) {
                                if (i > 8
                                    ? bytes[6] & (1 << (i - 9))
                                    : bytes[7] & (1 << (i - 1))) {
                                    return i;
                                }
                            }
                            break;
                    }
                    return bytes[5];

                default:
                    return bytes[6];
            }
        }

        uint8_t TxUltimateTouch::get_touch_state(const std::array<uint8_t, UART_BUFFER_SIZE> &bytes) const {
            uint8_t state = bytes[4];

            // State resolution logic from original implementations
            if (state == TOUCH_STATE_PRESS && bytes[5] != 0) {
                state = TOUCH_STATE_RELEASE;
            }

            if (state == TOUCH_STATE_RELEASE && (bytes[5] == TOUCH_STATE_ALL_FIELDS)) {
                state = bytes[5];
            }

            if (state == TOUCH_STATE_SWIPE) {
                if (bytes[5] == TOUCH_STATE_SWIPE_RIGHT) {
                    state = TOUCH_STATE_SWIPE_RIGHT;
                } else if (bytes[5] == TOUCH_STATE_SWIPE_LEFT) {
                    state = TOUCH_STATE_SWIPE_LEFT;
                }
            }

            return state;
        }

        TouchPoint TxUltimateTouch::get_touch_point(const std::array<uint8_t, UART_BUFFER_SIZE> &bytes) const {
            TouchPoint tp;
            tp.x = this->get_x_touch_position(bytes);
            tp.state = this->get_touch_state(bytes);
            tp.state_str = this->get_state_string(tp.state);
            
            return tp;
        }

        std::string TxUltimateTouch::get_state_string(uint8_t state) const {
            switch (state) {
                case TOUCH_STATE_RELEASE:
                    return "RELEASE";
                case TOUCH_STATE_PRESS:
                    return "PRESS";
                case TOUCH_STATE_SWIPE:
                    return "SWIPE";
                case TOUCH_STATE_ALL_FIELDS:
                    return "MULTI_TOUCH";
                case TOUCH_STATE_SWIPE_RIGHT:
                    return "SWIPE_RIGHT";
                case TOUCH_STATE_SWIPE_LEFT:
                    return "SWIPE_LEFT";
                default:
                    return "UNKNOWN_" + std::to_string(state);
            }
        }

    } // namespace tx_ultimate_touch
} // namespace esphome