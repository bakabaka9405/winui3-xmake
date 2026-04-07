#pragma once

#include "MainWindow.g.h"

namespace winrt::xmake_demo::implementation {
struct MainWindow : MainWindowT<MainWindow> {
	MainWindow();

	void myButton_Click(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);
};
} // namespace winrt::xmake_demo::implementation

namespace winrt::xmake_demo::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
};
} // namespace winrt::xmake_demo::factory_implementation
