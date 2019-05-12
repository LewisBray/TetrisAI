#include "currenttime.h"

std::chrono::microseconds currentTime()
{
    using namespace std::chrono;

    const auto clockTime = steady_clock().now();
    return duration_cast<microseconds>(clockTime.time_since_epoch());
}
