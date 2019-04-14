#include "currenttime.h"

std::chrono::milliseconds currentTime()
{
    using namespace std::chrono;

    const auto clockTime = steady_clock().now();
    return duration_cast<milliseconds>(clockTime.time_since_epoch());
}
