#pragma once

#include "HomePage.g.h"

namespace winrt::gallery::implementation {
struct HomePage : HomePageT<HomePage> {
    HomePage();
};
} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct HomePage : HomePageT<HomePage, implementation::HomePage> {
};
} // namespace winrt::gallery::factory_implementation
