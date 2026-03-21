#ifndef DECODE_H_
#define DECODE_H_

#include <cstdint>

class Time {
    public: 
        const bool summer_time;
        const bool time_change;
        const bool leap_second;
        //
        const uint8_t year;
        const uint8_t month;
        const uint8_t day;
        const uint8_t dow;
        const uint8_t hour;
        const uint8_t minute;
        const uint8_t parity_error_count;

        bool is_timewise_succ(const Time* succ, const int minute_difference) const;
        int get_clock_seconds() const;
        bool is_range_error() const;
};
    
#endif // DECODE_H_