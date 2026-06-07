#pragma once

#include <format>
#include <string_view>
#include <utility>

namespace demo::utils::debug_trace {
namespace detail {
void WriteFormatted(std::wstring_view source, std::wstring_view message);
}

template <typename... Args>
void Write(std::wstring_view source, std::wformat_string<Args...> format, Args&&... args) {
#if defined(_DEBUG)
	detail::WriteFormatted(source, std::format(format, std::forward<Args>(args)...));
#else
	(void)source;
	(void)format;
	if constexpr (sizeof...(Args) > 0) {
		((void)args, ...);
	}
#endif
}

} // namespace demo::utils::debug_trace
