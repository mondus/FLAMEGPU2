#include <thread>
#include <chrono>
#include "flamegpu/util/detail/SteadyClockTimer.h"

#include "gtest/gtest.h"
namespace flamegpu {


TEST(TestSteadyClockTimer, SteadyClockTimer) {
    // Create an event timer, time should be 0 initially.
    util::detail::SteadyClockTimer * timer = nullptr;
    EXPECT_NO_THROW(timer = new util::detail::SteadyClockTimer());

    // Time an arbitrary event, and check the value is approximately correct.
    timer->start();
    const int sleep_duration_seconds = 1;
    const int min_expected_millis = static_cast<int>(sleep_duration_seconds * 1000.0 * 0.9);
    std::this_thread::sleep_for(std::chrono::seconds(sleep_duration_seconds));
    timer->stop();
    EXPECT_GE(timer->getElapsedMilliseconds(), min_expected_millis);

    // Trigger the destructor.
    EXPECT_NO_THROW(delete timer);
}
}  // namespace flamegpu
