#include "decode.h"

bool Time::parity_check(uint8_t* buffer, int begin, int end) {
    bool parity = false;
    for (int i = begin; i <= end; i++) {
        if ((buffer[i / 8] & (0x01 << (i % 8))) != 0) {
            parity = !parity;
        }
    }
    return !parity;
}

bool Time::is_timewise_succ(Time* succ, int minute_difference) {
    int this_flat_mins = this->hour*60 + this->minute;
    int succ_flat_mins = succ->hour*60 + succ->minute;

    return ((this_flat_mins + minute_difference) % 1440) == succ_flat_mins;
}

Time* Time::decode_telegram(uint8_t* buffer) {
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
    
    return new Time {
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
