#include "PointerFilter.h"
#include "pch.h"


#include <cmath>

namespace winrt::paint::implementation {

static float ComputeAlpha(float dt, float cutoff) {
	float const tau = 1.0f / (2.0f * 3.14159265358979323846f * cutoff);
	return dt / (dt + tau);
}

winrt::Windows::Foundation::Point OneEuroFilter::Step(
	winrt::Windows::Foundation::Point const& raw,
	std::chrono::steady_clock::time_point const& now) {
	if (!initialized) {
		rawPrev = raw;
		filteredPrev = raw;
		dhatPrev = { 0, 0 };
		tPrev = now;
		initialized = true;
		return raw;
	}

	auto const dt = std::chrono::duration<float>(now - tPrev).count();
	if (dt <= 0.0f) return filteredPrev;

	// 先平滑速度，再用速度决定位置滤波强度。
	float const dx = (raw.X - rawPrev.X) / dt;
	float const dy = (raw.Y - rawPrev.Y) / dt;
	float const ad = ComputeAlpha(dt, dcutoff);
	float const dxh = dhatPrev.X + ad * (dx - dhatPrev.X);
	float const dyh = dhatPrev.Y + ad * (dy - dhatPrev.Y);

	float const speed = std::sqrt(dxh * dxh + dyh * dyh);
	float const cutoff = minCutoff + beta * speed;

	float const a = ComputeAlpha(dt, cutoff);
	float const xh = filteredPrev.X + a * (raw.X - filteredPrev.X);
	float const yh = filteredPrev.Y + a * (raw.Y - filteredPrev.Y);

	rawPrev = raw;
	filteredPrev = { xh, yh };
	dhatPrev = { dxh, dyh };
	tPrev = now;

	return filteredPrev;
}

} // namespace winrt::paint::implementation