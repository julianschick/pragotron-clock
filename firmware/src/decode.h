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
        static Time* decode_telegram(uint8_t* buffer);

    private:
        static bool parity_check(uint8_t* buffer, int begin, int end);
};
    
#endif // DECODE_H_