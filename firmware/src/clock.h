#ifndef CLOCK_H_
#define CLOCK_H_

#include <stdint.h>

class Clock {

public:
    Clock(uint32_t freq);
    ~Clock();

    void put_telegram(uint32_t begin, uint32_t len);

private: 
    uint32_t freq;
    uint32_t last_start_ms;
    int bit_counter;
    bool bits[59];
    bool last_complete_datagram[59];
};

#endif