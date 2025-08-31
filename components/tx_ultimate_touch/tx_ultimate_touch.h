#pragma once
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/uart/uart_component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/light/addressable_light.h"
#include <array>
#include <string>
#include <vector>

namespace esphome {
    namespace tx_ultimate_touch {
        // UART Protocol Constants
        constexpr int UART_BUFFER_SIZE = 15;
        constexpr uint8_t HEADER_BYTE_1 = 0xAA;        // 170
        constexpr uint8_t HEADER_BYTE_2 = 0x55;        // 85
        constexpr uint8_t PACKET_VERSION = 0x01;       // 1
        constexpr uint8_t PACKET_OPCODE = 0x02;        // 2
        constexpr uint8_t ERROR_RESPONSE_VERSION = 0x01;
        constexpr uint8_t ERROR_RESPONSE_OPCODE = 0x82; // 130
        
        // Touch State Constants
        constexpr uint8_t TOUCH_STATE_RELEASE = 0x01;
        constexpr uint8_t TOUCH_STATE_PRESS = 0x02;
        constexpr uint8_t TOUCH_STATE_SWIPE = 0x03;
        constexpr uint8_t TOUCH_STATE_ALL_FIELDS = 0x0B;      // 11 - kept for compatibility
        constexpr uint8_t TOUCH_STATE_MULTI_TOUCH = 0x0B;     // 11 - same as ALL_FIELDS
        constexpr uint8_t TOUCH_STATE_SWIPE_RIGHT = 0x0C;     // 12
        constexpr uint8_t TOUCH_STATE_SWIPE_LEFT = 0x0D;      // 13
        
        // Position Constants
        constexpr uint8_t TOUCH_MAX_POSITION = 10;
        constexpr uint8_t LONG_PRESS_OFFSET = 16;
        
        // Timing Constants - SOLUȚIA HIBRIDĂ CU STATE MACHINE
        constexpr uint16_t PACKET_TIMEOUT_MS = 100;
        constexpr uint16_t DEBOUNCE_TIME_MS = 50;
        constexpr uint16_t AUTO_RELEASE_TIMEOUT = 500;        // Auto-release la 700ms
        constexpr uint16_t LONG_PRESS_TIMEOUT = 3000;          // Timeout final la 3000ms (mai scurt)
        constexpr uint16_t LONG_PRESS_THRESHOLD = 1200;        // Pragul pentru long press (1200ms)
        
        // CRC Constants
        constexpr uint16_t CRC16_POLY = 0x1021;  // CRC-16/CCITT-FALSE polynomial
        constexpr uint16_t CRC16_INIT = 0xFFFF;  // Initial CRC value
        
        // Special Values
        constexpr uint8_t INVALID_VALUE = 255;

        // STATE MACHINE pentru touch events
        enum TouchStateMachine {
            TOUCH_IDLE = 0,           // Waiting for touch
            TOUCH_PRESSED = 1,        // Touch detected, waiting for release or timeout
            TOUCH_AUTO_RELEASED = 2   // Auto-release sent, waiting for real release or timeout
        };

        static const char *TAG = "tx_ultimate_touch";

        // Enhanced TouchPoint structure with swipe details
        struct TouchPoint {
            uint8_t x = INVALID_VALUE;           // Position (1-10)
            uint8_t from_pos = INVALID_VALUE;    // Start position for swipe
            uint8_t to_pos = INVALID_VALUE;      // End position for swipe
            uint8_t state = INVALID_VALUE;       // Touch state
            std::string state_str = "Unknown";   // Human readable state
            bool crc_valid = false;              // CRC validation status
        };

        class TxUltimateTouch : public uart::UARTDevice, public Component {
        public:
            // Trigger getters - combining all implementations
            Trigger<TouchPoint> *get_touch_trigger() { return &this->touch_trigger_; }
            Trigger<TouchPoint> *get_release_trigger() { return &this->release_trigger_; }
            Trigger<TouchPoint> *get_swipe_left_trigger() { return &this->swipe_trigger_left_; }
            Trigger<TouchPoint> *get_swipe_right_trigger() { return &this->swipe_trigger_right_; }
            Trigger<TouchPoint> *get_full_touch_release_trigger() { return &this->full_touch_release_trigger_; }
            Trigger<TouchPoint> *get_multi_touch_release_trigger() { return &this->multi_touch_release_trigger_; }
            Trigger<TouchPoint> *get_long_touch_release_trigger() { return &this->long_touch_release_trigger_; }
            Trigger<TouchPoint> *get_touch_event_trigger() { return &this->touch_event_trigger_; }

            // UART component setter
            void set_uart_component(esphome::uart::UARTComponent *uart_component) {
                this->set_uart_parent(uart_component);
            }
            
            // Component lifecycle methods
            void setup() override;
            void loop() override;
            void dump_config() override;

        protected:
            // Core processing methods - optimized signatures
            void send_touch_(TouchPoint tp);
            void handle_touch(const std::vector<uint8_t> &packet);
            TouchPoint get_touch_point(const std::vector<uint8_t> &packet) const;
            bool is_valid_data(const std::vector<uint8_t> &packet) const;
            uint8_t get_x_touch_position(const std::vector<uint8_t> &packet) const;
            uint8_t get_touch_state(const std::vector<uint8_t> &packet) const;
            std::string get_state_string(uint8_t state) const;
            
            // CRC validation methods
            uint16_t calculate_crc16(const uint8_t* data, size_t length, uint16_t poly = CRC16_POLY) const;
            bool validate_crc16(const std::vector<uint8_t> &packet) const;
            
            // Enhanced packet processing methods
            void process_uart_packets();
            void process_multiple_messages(std::vector<uint8_t> &buffer);
            std::vector<std::vector<uint8_t>> split_packets(const std::vector<uint8_t> &buffer);
            void dump_packet_hex(const std::vector<uint8_t> &packet, const std::string &label = "") const;
            
            // Enhanced swipe detection
            void extract_swipe_positions(const std::vector<uint8_t> &packet, TouchPoint &tp) const;
            
            // STATE MACHINE methods
            void handle_touch_state_machine();
            void transition_to_state(TouchStateMachine new_state);
            void force_release();
            void handle_release_event(TouchPoint tp, unsigned long current_time);
            void simulate_touch_sequence(uint8_t position);
            void reset_touch_state();

            // Event triggers - comprehensive set
            Trigger<TouchPoint> touch_trigger_;              // Press events
            Trigger<TouchPoint> release_trigger_;            // Release events  
            Trigger<TouchPoint> swipe_trigger_left_;         // Swipe left
            Trigger<TouchPoint> swipe_trigger_right_;        // Swipe right
            Trigger<TouchPoint> full_touch_release_trigger_; // Full touch release (compatibility)
            Trigger<TouchPoint> multi_touch_release_trigger_;// Multi-touch release
            Trigger<TouchPoint> long_touch_release_trigger_; // Long press release
            Trigger<TouchPoint> touch_event_trigger_;        // All events (from tx_ultimate_easy)

            // STATE MACHINE variables
            TouchStateMachine touch_state_ = TOUCH_IDLE;         // Current state machine state
            unsigned long state_start_time_ = 0;                 // Time when current state started
            uint8_t last_press_x_ = INVALID_VALUE;               // Last press position (for long press)
            
            // Buffer management
            std::vector<uint8_t> uart_buffer_;                   // Persistent UART buffer
        }; // class TxUltimateTouch

    } // namespace tx_ultimate_touch
} // namespace esphome