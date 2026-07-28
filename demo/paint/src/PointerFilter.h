#pragma once

#include "pch.h"

namespace winrt::paint::implementation {

// One Euro Filter 根据笔触速度调整截止频率：慢速强滤波，快速弱滤波。
struct OneEuroFilter {
	float minCutoff = 1.0f;
	float beta = 0.007f;
	float dcutoff = 1.0f;

	bool initialized = false;
	winrt::Windows::Foundation::Point rawPrev{};
	winrt::Windows::Foundation::Point filteredPrev{};
	winrt::Windows::Foundation::Point dhatPrev{};
	std::chrono::steady_clock::time_point tPrev{};

	void Reset() { initialized = false; }

	winrt::Windows::Foundation::Point Step(
		winrt::Windows::Foundation::Point const& raw,
		std::chrono::steady_clock::time_point const& now);
};

} // namespace winrt::paint::implementation
