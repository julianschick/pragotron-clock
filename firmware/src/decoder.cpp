#include "decoder.h"

#include <Arduino.h>
#include "defines.h"
#include "sync.h"

void Decoder::next(const uint32_t rx_time, const uint32_t signal_duration) {
    int bit = -1;
    if (signal_duration >= MINIMAL_ZERO_TIME && signal_duration <= MAXIMAL_ZERO_TIME) {
        bit = 0;
    }
    if (signal_duration >= MINIMAL_ONE_TIME && signal_duration <= MAXIMAL_ONE_TIME) {
        bit = 1;
    }

    if (bit == -1) {
        invalid_bit = true;
    } else {
        unsigned long rx_time_diff = rx_time - last_bit_rx_time;
        last_bit_rx_time = rx_time;

        if (cursor >= 59 || (rx_time_diff >= MINIMAL_MINUTE_PAUSE_TIME && rx_time_diff <= MAXIMAL_MINUTE_PAUSE_TIME)) {
            cursor = 0;
        }

        buffer[cursor / 8] = (bit == 1)
            ? buffer[cursor / 8] | (0x01 << (cursor % 8))
            : buffer[cursor / 8] & (~(0x01 << (cursor % 8)));
        cursor++;

        #ifdef DEBUG_FINE
        if (sync->get_clock_seconds() == -1) {
            Serial.printf("buf[%d] = %d\n", cursor - 1, bit);
        }
        #endif

        #ifdef DEBUG_FINEST
        Serial.println("-----------------");
        Serial.printf("delta_t = %ld; sig_duration = %d; buffer[%d] = %d\n", rx_time_diff, signal_duration, cursor - 1, bit);
        Serial.printf("clock_seconds=%d\n", Sync::get_clock_seconds());
        Serial.printf("timer1_at_flank_up=%d\n", Sync::get_timer1_at_flank_up());
        #endif

        if (cursor == 59) {
            Time t = decode_buffer();
                        
            // if (last_accepted_time != NULL) {
            //     minute_diff = static_cast<int>(round(static_cast<double>(Sync::get_clock_seconds() - last_accepted_time->get_clock_seconds()) / 60.0));
            // }

            bool accept_anyway = false;
            bool erroneous = t.parity_error_count > 0 || invalid_bit || t.is_range_error();
            //bool suspicious_diff = !(last_accepted_time == NULL || last_accepted_time->is_timewise_succ(t, minute_diff));

            const int cs = sync->get_clock_seconds();
            const int d = cs - t.get_clock_seconds();
            bool suspicious_time = cs != -1 
                && modulo(d, OVERFLOW_SECS) >= SUSPICIOUS_SECOND_DIFF_THRS
                && modulo(-d, OVERFLOW_SECS) >= SUSPICIOUS_SECOND_DIFF_THRS;
            
            if (suspicious_time) {
                suspicious_time_counter++;

                if (suspicious_time_counter > MAX_SUSPICIOUS_TIME_FRAMES) {
                   accept_anyway = true;
                   suspicious_time_counter = 0;
                }
            }

            bool accept = accept_anyway || !(suspicious_time || erroneous);

            if (accept) {
                // if (last_accepted_time != NULL) {
                //     delete last_accepted_time;
                // }
                // last_accepted_time = t;
                sync->set_next_second(t.get_clock_seconds() - 1);
            }

            #ifdef DEBUG
            if (!accept) {
                Serial.printf("[NOT ACCEPTED %d(%d%d%d)%d] %02d-%02d-%02d %02d:%02d %s, dow = %d, err_count=%d, stc=%d\n", 
                    erroneous, t.parity_error_count, invalid_bit, t.is_range_error(), suspicious_time,
                    t.year, t.month, t.day, t.hour, t.minute, t.summer_time ? "MESZ" : "MEZ", t.dow, t.parity_error_count, suspicious_time_counter
                );
            } else {
                Serial.printf("%02d-%02d-%02d %02d:%02d %s, dow = %d, err_count=%d, stc=%d\n", 
                    t.year, t.month, t.day, t.hour, t.minute, t.summer_time ? "MESZ" : "MEZ", t.dow, t.parity_error_count, suspicious_time_counter
                );
            }
            Serial.flush();
            #endif

            invalid_bit = false;
        }
    }
}


Time Decoder::decode_buffer() {
    uint8_t parity_error_count = 0;
    
    bool time_change = (buffer[2] & 0x01) != 0;
    bool summer_time = ((buffer[2] & 0b00000110) >> 1) == 0b01;
    bool leap_second = (buffer[2] & 0b00001000) != 0;

    uint8_t min_one = ((0b11100000 & buffer[2]) >> 5) | ((0x01 & buffer[3]) << 3);
    uint8_t min_ten = (0b00001110 & buffer[3]) >> 1;
    if (!parity_check(buffer, 21, 28)) {
        parity_error_count++;
    }

    uint8_t hour_one = ((0b11100000 & buffer[3]) >> 5) | ((0x01 & buffer[4]) << 3);
    uint8_t hour_ten = (0b00000110 & buffer[4]) >> 1;

    if (!parity_check(buffer, 29, 35)) {
        parity_error_count++;
    }

    uint8_t day_one = (0b11110000 & buffer[4]) >> 4;
    uint8_t day_ten = 0b00000011 & buffer[5];
     
    uint8_t dow = (0b00011100 & buffer[5]) >> 2;

    uint8_t month_one = ((0b11100000 & buffer[5]) >> 5) | ((0x01 & buffer[6]) << 3);
    uint8_t month_ten = (buffer[6] & 0b10) >> 1;

    uint8_t year_one = (0b00111100 & buffer[6]) >> 2;
    uint8_t year_ten = ((0b11000000 & buffer[6]) >> 6) | ((0b00000011 & buffer[7]) << 2);

    if (!parity_check(buffer, 36, 58)) {
        parity_error_count++;
    }
    
    return Time {
        summer_time, time_change, leap_second,
        static_cast<uint8_t>(year_one + 10*year_ten),
        static_cast<uint8_t>(month_one + 10*month_ten),
        static_cast<uint8_t>(day_one + 10*day_ten),
        dow,
        static_cast<uint8_t>(hour_one + 10*hour_ten),
        static_cast<uint8_t>(min_one + 10*min_ten),
        parity_error_count
    };
}

bool Decoder::parity_check(const uint8_t* buffer, const int begin, const int end) {
    bool parity = false;
    for (int i = begin; i <= end; i++) {
        if ((buffer[i / 8] & (0x01 << (i % 8))) != 0) {
            parity = !parity;
        }
    }
    return !parity;
}
