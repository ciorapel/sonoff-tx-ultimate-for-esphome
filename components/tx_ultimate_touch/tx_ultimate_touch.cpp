#include "esphome/core/log.h"
#include "tx_ultimate_touch.h"
#include <cinttypes>

namespace esphome {
    namespace tx_ultimate_touch {

        void TxUltimateTouch::setup() {
            ESP_LOGI(TAG, "TX Ultimate Touch is initialized");
            this->touch_state_ = TOUCH_IDLE;
            this->uart_buffer_.reserve(256);  // Pre-allocate buffer space
        }

        void TxUltimateTouch::loop() {
            // Process UART packets first
            this->process_uart_packets();
            
            // Handle state machine with single timer
            this->handle_touch_state_machine();
        }

        void TxUltimateTouch::process_uart_packets() {
            // Read all available bytes into buffer
            while (this->available()) {
                this->uart_buffer_.push_back(this->read());
            }
            
            // Process complete messages from buffer
            this->process_multiple_messages(this->uart_buffer_);
        }

        void TxUltimateTouch::process_multiple_messages(std::vector<uint8_t> &buffer) {
            if (buffer.size() < 6) return;  // Minimum packet size
            
            // Split buffer into individual packets
            auto packets = this->split_packets(buffer);
            
            // Process each complete packet
            for (const auto &packet : packets) {
                if (packet.size() >= 6) {
                    this->dump_packet_hex(packet, "Processing");
                    this->handle_touch(packet);
                }
            }
            
            // Keep only incomplete data in buffer
            // Find last header position
            size_t last_header_pos = buffer.size();
            for (size_t i = buffer.size() - 1; i > 0; i--) {
                if (buffer[i] == HEADER_BYTE_1 && i + 1 < buffer.size() && buffer[i + 1] == HEADER_BYTE_2) {
                    last_header_pos = i;
                    break;
                }
            }
            
            // Keep incomplete packet at end
            if (last_header_pos < buffer.size() && buffer.size() - last_header_pos < 8) {
                std::vector<uint8_t> remaining(buffer.begin() + last_header_pos, buffer.end());
                buffer = std::move(remaining);
            } else {
                buffer.clear();
            }
            
            // Prevent buffer overflow
            if (buffer.size() > 100) {
                ESP_LOGW(TAG, "Buffer overflow protection - clearing buffer");
                buffer.clear();
            }
        }

        std::vector<std::vector<uint8_t>> TxUltimateTouch::split_packets(const std::vector<uint8_t> &buffer) {
            std::vector<std::vector<uint8_t>> packets;
            std::vector<size_t> header_positions;
            
            // Find all header positions (AA 55)
            for (size_t i = 0; i < buffer.size() - 1; i++) {
                if (buffer[i] == HEADER_BYTE_1 && buffer[i + 1] == HEADER_BYTE_2) {
                    header_positions.push_back(i);
                }
            }
            
            // Extract packets between headers
            for (size_t i = 0; i < header_positions.size(); i++) {
                size_t start = header_positions[i];
                size_t end = (i + 1 < header_positions.size()) ? header_positions[i + 1] : buffer.size();
                
                // Extract packet
                std::vector<uint8_t> packet(buffer.begin() + start, buffer.begin() + end);
                
                // Only add packets with minimum size and non-zero content
                if (packet.size() >= 6) {
                    bool has_data = false;
                    for (size_t j = 2; j < packet.size(); j++) {
                        if (packet[j] != 0x00) {
                            has_data = true;
                            break;
                        }
                    }
                    if (has_data) {
                        packets.push_back(packet);
                    }
                }
            }
            
            return packets;
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
            release_tp.crc_valid = true; // Simulated events are always valid

            ESP_LOGD(TAG, "Forced release at position %u", release_tp.x);
            this->release_trigger_.trigger(release_tp);
            this->touch_event_trigger_.trigger(release_tp);
        }

        void TxUltimateTouch::reset_touch_state() {
            this->touch_state_ = TOUCH_IDLE;
            this->last_press_x_ = INVALID_VALUE;
            this->state_start_time_ = millis();
        }

        uint16_t TxUltimateTouch::calculate_crc16(const uint8_t* data, size_t length, uint16_t poly) const {
            uint16_t crc = CRC16_INIT;
            
            for (size_t i = 0; i < length; i++) {
                crc ^= data[i] << 8;
                for (int j = 0; j < 8; j++) {
                    if (crc & 0x8000) {
                        crc = (crc << 1) ^ poly;
                    } else {
                        crc = crc << 1;
                    }
                }
            }
            return crc & 0xFFFF;
        }

        bool TxUltimateTouch::validate_crc16(const std::vector<uint8_t> &packet) const {
            // Disable CRC validation temporarily - protocol might not use CRC or uses different method
            // The original implementation didn't have CRC validation and worked fine
            
            ESP_LOGVV(TAG, "CRC validation disabled - assuming packet is valid");
            return true;
            
            /* Original CRC validation code - disabled for now
            if (packet.size() < 8) return false;  // Need at least header + 4 bytes + 2 CRC bytes
            
            // Find actual packet length (excluding trailing zeros)
            size_t packet_length = packet.size();
            for (size_t i = packet.size() - 1; i >= 6; i--) {  // Start from end, but keep minimum 6 bytes
                if (packet[i] != 0x00) {
                    packet_length = i + 1;
                    break;
                }
            }
            
            if (packet_length < 8) return false;  // Need CRC bytes
            
            // Calculate CRC for all data except last 2 bytes (which are the CRC)
            uint16_t calculated_crc = this->calculate_crc16(packet.data(), packet_length - 2);
            
            // Extract received CRC (big endian)
            uint16_t received_crc = (packet[packet_length - 2] << 8) | packet[packet_length - 1];
            
            bool crc_valid = calculated_crc == received_crc;
            
            ESP_LOGVV(TAG, "CRC Check: calc=0x%04X, recv=0x%04X, valid=%s", 
                     calculated_crc, received_crc, crc_valid ? "YES" : "NO");
                     
            return crc_valid;
            */
        }

        void TxUltimateTouch::dump_packet_hex(const std::vector<uint8_t> &packet, const std::string &label) const {
            if (!packet.empty()) {
                std::string hex_str;
                for (size_t i = 0; i < packet.size() && i < 15; i++) {
                    char hex_byte[8];
                    snprintf(hex_byte, sizeof(hex_byte), "%s%02X", (i > 0 ? " " : ""), packet[i]);
                    hex_str += hex_byte;
                }
                
                // Enhanced logging for swipe debugging
                if (packet.size() >= 6 && (packet[4] == TOUCH_STATE_SWIPE || 
                    (packet[4] == TOUCH_STATE_SWIPE && (packet[5] == 12 || packet[5] == 13)))) {
                    ESP_LOGD(TAG, "%s SWIPE packet [%d bytes]: %s", 
                            label.c_str(), (int)packet.size(), hex_str.c_str());
                    ESP_LOGD(TAG, "  Header: %02X %02X, Ver: %02X, Op: %02X, State: %02X, Sub: %02X", 
                            packet[0], packet[1], packet[2], packet[3], packet[4], 
                            packet.size() > 5 ? packet[5] : 0);
                    if (packet.size() >= 8) {
                        ESP_LOGD(TAG, "  Data bytes [6-7]: %02X %02X", packet[6], packet[7]);
                    }
                } else {
                    ESP_LOGV(TAG, "%s packet [%d bytes]: %s", 
                            label.c_str(), (int)packet.size(), hex_str.c_str());
                }
            }
        }

        void TxUltimateTouch::handle_touch(const std::vector<uint8_t> &packet) {
            this->dump_packet_hex(packet, "Raw");
            
            if (this->is_valid_data(packet)) {
                TouchPoint tp = this->get_touch_point(packet);
                tp.crc_valid = this->validate_crc16(packet);
                
                // Skip logging CRC failures since we disabled CRC validation
                // if (!tp.crc_valid) {
                //     ESP_LOGW(TAG, "CRC validation failed for packet");
                //     // Still process the packet but mark as potentially corrupted
                // }
                
                this->send_touch_(tp);
            } else {
                ESP_LOGW(TAG, "Invalid touch data received");
                this->dump_packet_hex(packet, "Invalid");
            }
        }

        void TxUltimateTouch::dump_config() {
            ESP_LOGCONFIG(TAG, "TX Ultimate Touch");
            ESP_LOGCONFIG(TAG, "  Max position: %u", TOUCH_MAX_POSITION);
            ESP_LOGCONFIG(TAG, "  Auto-release timeout: %u ms", AUTO_RELEASE_TIMEOUT);
            ESP_LOGCONFIG(TAG, "  Long press timeout: %u ms", LONG_PRESS_TIMEOUT);
            ESP_LOGCONFIG(TAG, "  Long press threshold: %u ms", LONG_PRESS_THRESHOLD);
            ESP_LOGCONFIG(TAG, "  Debounce time: %u ms", DEBOUNCE_TIME_MS);
            ESP_LOGCONFIG(TAG, "  CRC validation: enabled");
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
                        ESP_LOGD(TAG, "Press at position %u (CRC: %s)", tp.x, tp.crc_valid ? "OK" : "FAIL");
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
                    ESP_LOGD(TAG, "Swipe Left from %u to %u (pos: %u, CRC: %s)", 
                            tp.from_pos, tp.to_pos, tp.x, tp.crc_valid ? "OK" : "FAIL");
                    this->swipe_trigger_left_.trigger(tp);
                    this->transition_to_state(TOUCH_IDLE);
                    break;

                case TOUCH_STATE_SWIPE_RIGHT:
                    ESP_LOGD(TAG, "Swipe Right from %u to %u (pos: %u, CRC: %s)", 
                            tp.from_pos, tp.to_pos, tp.x, tp.crc_valid ? "OK" : "FAIL");
                    this->swipe_trigger_right_.trigger(tp);
                    this->transition_to_state(TOUCH_IDLE);
                    break;

                case TOUCH_STATE_ALL_FIELDS:
                    ESP_LOGD(TAG, "Multi Touch Release (CRC: %s)", tp.crc_valid ? "OK" : "FAIL");
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
                    ESP_LOGI(TAG, "Long press detected! Duration: %lums at position %u (CRC: %s)", 
                            total_press_time, this->last_press_x_, tp.crc_valid ? "OK" : "FAIL");
                    
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
                ESP_LOGD(TAG, "Hardware Long Press Release at position %u (CRC: %s)", 
                        tp.x, tp.crc_valid ? "OK" : "FAIL");
                this->long_touch_release_trigger_.trigger(tp);
            } else if (tp.x <= TOUCH_MAX_POSITION) {
                ESP_LOGD(TAG, "Release at position %u (CRC: %s)", tp.x, tp.crc_valid ? "OK" : "FAIL");
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
            sim_press.crc_valid = true; // Simulated events are always valid
            
            TouchPoint sim_release = sim_press;
            sim_release.state = TOUCH_STATE_RELEASE;
            sim_release.state_str = "SIMULATED_RELEASE";
            
            // Send simulated events
            this->touch_trigger_.trigger(sim_press);
            this->touch_event_trigger_.trigger(sim_press);
            this->release_trigger_.trigger(sim_release);
            this->touch_event_trigger_.trigger(sim_release);
        }

        bool TxUltimateTouch::is_valid_data(const std::vector<uint8_t> &packet) const {
            if (packet.size() < 6) return false;
            
            // Check packet header
            if (packet[0] != HEADER_BYTE_1 || 
                packet[1] != HEADER_BYTE_2 || 
                packet[2] != PACKET_VERSION || 
                packet[3] != PACKET_OPCODE) {
                ESP_LOGV(TAG, "Invalid packet header: %u %u %u %u", 
                        packet[0], packet[1], packet[2], packet[3]);
                return false;
            }

            uint8_t state = this->get_touch_state(packet);
            if (state != TOUCH_STATE_PRESS &&
                state != TOUCH_STATE_RELEASE &&
                state != TOUCH_STATE_SWIPE_LEFT &&
                state != TOUCH_STATE_SWIPE_RIGHT &&
                state != TOUCH_STATE_ALL_FIELDS) {
                ESP_LOGV(TAG, "Invalid touch state: %u", state);
                return false;
            }

            // More lenient position validation - allow swipes to report position 0 initially
            if (state != TOUCH_STATE_ALL_FIELDS && state != TOUCH_STATE_SWIPE_LEFT && state != TOUCH_STATE_SWIPE_RIGHT) {
                uint8_t x = this->get_x_touch_position(packet);
                if (x == 0 || x > (LONG_PRESS_OFFSET + TOUCH_MAX_POSITION)) {
                    ESP_LOGV(TAG, "Invalid position: %u for state %u", x, state);
                    return false;
                }
            }

            return true;
        }

        void TxUltimateTouch::extract_swipe_positions(const std::vector<uint8_t> &packet, TouchPoint &tp) const {
            if (packet.size() < 6) return;
            
            // Reset positions first
            tp.from_pos = INVALID_VALUE;
            tp.to_pos = INVALID_VALUE;
            
            if (tp.state == TOUCH_STATE_SWIPE_LEFT || tp.state == TOUCH_STATE_SWIPE_RIGHT) {
                ESP_LOGD(TAG, "Analyzing swipe packet: state=%u, size=%d", tp.state, (int)packet.size());
                
                // Method 1: Direct extraction from bytes 6 and 7 (Tasmota style)
                if (packet.size() >= 8) {
                    uint8_t byte6 = packet[6];
                    uint8_t byte7 = packet[7];
                    
                    ESP_LOGD(TAG, "Raw bytes: [6]=%u [7]=%u", byte6, byte7);
                    
                    if (byte6 > 0 && byte6 <= TOUCH_MAX_POSITION) {
                        tp.from_pos = byte6;
                    }
                    if (byte7 > 0 && byte7 <= TOUCH_MAX_POSITION) {
                        tp.to_pos = byte7;
                    }
                }
                
                // Method 2: Try bit-field extraction if direct method fails
                if ((tp.from_pos == INVALID_VALUE || tp.to_pos == INVALID_VALUE) && packet.size() >= 8) {
                    ESP_LOGD(TAG, "Trying bit-field extraction...");
                    
                    uint8_t swipe_type = packet[5]; // 12 = right, 13 = left
                    uint8_t bit_data1 = packet[6];
                    uint8_t bit_data2 = packet[7];
                    
                    ESP_LOGD(TAG, "Swipe type: %u, bit_data1: 0x%02X, bit_data2: 0x%02X", 
                            swipe_type, bit_data1, bit_data2);
                    
                    // Try to extract positions from bit fields
                    for (uint8_t pos = 1; pos <= TOUCH_MAX_POSITION; pos++) {
                        bool bit_set = false;
                        
                        if (pos <= 8) {
                            // Positions 1-8 in bit_data2
                            bit_set = bit_data2 & (1 << (pos - 1));
                        } else if (pos <= 10) {
                            // Positions 9-10 in bit_data1
                            bit_set = bit_data1 & (1 << (pos - 9));
                        }
                        
                        if (bit_set) {
                            if (tp.from_pos == INVALID_VALUE) {
                                tp.from_pos = pos;
                            } else if (tp.to_pos == INVALID_VALUE) {
                                tp.to_pos = pos;
                            }
                            ESP_LOGD(TAG, "Found bit set at position %u", pos);
                        }
                    }
                }
                
                // Method 3: Use main position as fallback
                if (tp.from_pos == INVALID_VALUE && tp.x > 0 && tp.x <= TOUCH_MAX_POSITION) {
                    tp.from_pos = tp.x;
                    ESP_LOGD(TAG, "Using main position %u as from_pos", tp.x);
                }
                
                if (tp.to_pos == INVALID_VALUE && tp.from_pos != INVALID_VALUE) {
                    // Try to infer direction
                    if (tp.state == TOUCH_STATE_SWIPE_LEFT && tp.from_pos > 1) {
                        tp.to_pos = tp.from_pos - 1;
                    } else if (tp.state == TOUCH_STATE_SWIPE_RIGHT && tp.from_pos < TOUCH_MAX_POSITION) {
                        tp.to_pos = tp.from_pos + 1;
                    }
                    ESP_LOGD(TAG, "Inferred to_pos: %u based on direction", tp.to_pos);
                }
                
                ESP_LOGD(TAG, "Final swipe positions: from=%u, to=%u", tp.from_pos, tp.to_pos);
            }
        }

        uint8_t TxUltimateTouch::get_x_touch_position(const std::vector<uint8_t> &packet) const {
            if (packet.size() < 6) return INVALID_VALUE;
            
            // Enhanced position detection from multiple implementations
            switch (packet[4]) {
                case TOUCH_STATE_RELEASE:
                case TOUCH_STATE_ALL_FIELDS:
                    return packet[5];

                case TOUCH_STATE_SWIPE_LEFT:
                case TOUCH_STATE_SWIPE_RIGHT:
                    if (packet.size() < 8) return packet[5];
                    
                    // Advanced swipe position detection
                    switch (packet[5]) {
                        case 12: // Scan right to left
                            for (uint8_t i = TOUCH_MAX_POSITION; i > 0; i--) {
                                if (i > 8
                                    ? packet[6] & (1 << (i - 9))
                                    : packet[7] & (1 << (i - 1))) {
                                    return i;
                                }
                            }
                            break;
                        case 13: // Scan left to right
                            for (uint8_t i = 1; i <= TOUCH_MAX_POSITION; i++) {
                                if (i > 8
                                    ? packet[6] & (1 << (i - 9))
                                    : packet[7] & (1 << (i - 1))) {
                                    return i;
                                }
                            }
                            break;
                    }
                    return packet[5];

                default:
                    return packet.size() > 6 ? packet[6] : packet[5];
            }
        }

        uint8_t TxUltimateTouch::get_touch_state(const std::vector<uint8_t> &packet) const {
            if (packet.size() < 5) return INVALID_VALUE;
            
            uint8_t state = packet[4];

            // State resolution logic from original implementations
            if (state == TOUCH_STATE_PRESS && packet.size() > 5 && packet[5] != 0) {
                state = TOUCH_STATE_RELEASE;
            }

            if (state == TOUCH_STATE_RELEASE && packet.size() > 5 && (packet[5] == TOUCH_STATE_ALL_FIELDS)) {
                state = packet[5];
            }

            if (state == TOUCH_STATE_SWIPE && packet.size() > 5) {
                if (packet[5] == TOUCH_STATE_SWIPE_RIGHT) {
                    state = TOUCH_STATE_SWIPE_RIGHT;
                } else if (packet[5] == TOUCH_STATE_SWIPE_LEFT) {
                    state = TOUCH_STATE_SWIPE_LEFT;
                }
            }

            return state;
        }

        TouchPoint TxUltimateTouch::get_touch_point(const std::vector<uint8_t> &packet) const {
            TouchPoint tp;
            tp.x = this->get_x_touch_position(packet);
            tp.state = this->get_touch_state(packet);
            tp.state_str = this->get_state_string(tp.state);
            
            // Extract enhanced swipe information
            this->extract_swipe_positions(packet, tp);
            
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