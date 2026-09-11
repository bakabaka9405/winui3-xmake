#pragma once

#include <winrt/WinUIEditor.h>

#include "MainWindow.g.h"

namespace winrt::markdown::implementation {
struct MainWindow : MainWindowT<MainWindow> {
	MainWindow();

	void RefreshPreview_Click(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);
	void MarkdownEditor_Loaded(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);

private:
	void RenderPreview();
};
} // namespace winrt::markdown::implementation

namespace winrt::markdown::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
};
} // namespace winrt::markdown::factory_implementation
