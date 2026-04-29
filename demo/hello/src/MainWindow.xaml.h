#pragma once

#include "MainWindow.g.h"

namespace winrt::hello::implementation {
struct MainWindow : MainWindowT<MainWindow> {
	MainWindow();

	void myButton_Click(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);
};
} // namespace winrt::hello::implementation

namespace winrt::hello::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
};
} // namespace winrt::hello::factory_implementation
