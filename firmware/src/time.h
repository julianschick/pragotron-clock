#ifndef DECODE_H_
#define DECODE_H_

#include <cstdint>

class Time {
    public: 
        bool summer_time;
        bool time_change;
        bool leap_second;
        //
        uint8_t year;
        uint8_t month;
        uint8_t day;
        uint8_t dow;
        uint8_t hour;
        uint8_t minute;
        uint8_t parity_error_count;

        bool is_timewise_succ(Time* succ, int minute_difference);
        int get_clock_seconds();
        bool is_range_error();
};
    
#endif // DECODE_H_