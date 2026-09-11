#pragma once

#include <winrt/Microsoft.UI.Xaml.Controls.h>

namespace winrt::markdown::implementation {
struct MarkdownRenderer final {
	static Microsoft::UI::Xaml::Controls::StackPanel Render(winrt::hstring const& content, float dpi);
};
} // namespace winrt::markdown::implementation
