#include <chrono>
#include <cstdio>
#include <demo/utils/DebugTrace.h>
#include <windows.h>

namespace demo::utils::debug_trace::detail {

#if defined(_DEBUG)

auto const TraceStart = std::chrono::steady_clock::now();

bool EnsureConsole() {
	static bool initialized = false;
	static bool available = false;
	if (initialized) return available;

	initialized = true;
	available = AttachConsole(ATTACH_PARENT_PROCESS) || GetLastError() == ERROR_ACCESS_DENIED;
	if (available) {
		FILE* stream{};
		freopen_s(&stream, "CONOUT$", "w", stderr);
		setvbuf(stderr, nullptr, _IONBF, 0);
	}
	return available;
}

double ElapsedMilliseconds() {
	auto const elapsed = std::chrono::steady_clock::now() - TraceStart;
	return std::chrono::duration<double, std::milli>(elapsed).count();
}

#endif

void WriteFormatted(std::wstring_view source, std::wstring_view message) {
#if defined(_DEBUG)
	if (EnsureConsole()) {
		fwprintf(
			stderr,
			L"[%10.3f ms][tid %lu][%.*ls] %.*ls\n",
			ElapsedMilliseconds(),
			GetCurrentThreadId(),
			static_cast<int>(source.size()),
			source.data(),
			static_cast<int>(message.size()),
			message.data());
	}
#else
	(void)source;
	(void)message;
#endif
}

} // namespace demo::utils::debug_trace::detail
