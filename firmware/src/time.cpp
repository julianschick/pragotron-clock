#include "time.h"

bool Time::is_timewise_succ(const Time* succ, const int minute_difference) const {
    int this_flat_mins = this->hour*60 + this->minute;
    int succ_flat_mins = succ->hour*60 + succ->minute;

    return ((this_flat_mins + minute_difference) % 1440) == succ_flat_mins;
}

int Time::get_clock_seconds() const {
    return 3600*hour + 60*minute;
}

bool Time::is_range_error() const {
    return month == 0 || month > 12 
        || day == 0 || day > 31 
        || dow == 0 || dow > 7
        || hour > 23 
        || minute > 60;
}
