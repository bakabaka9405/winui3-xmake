#pragma once

#include "MainWindow.g.h"

namespace winrt::kanban::implementation {
struct MainWindow : MainWindowT<MainWindow> {
	MainWindow();

	void OnAddColumn(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& e);

private:
	void OnRootPointerPressed(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
};
} // namespace winrt::kanban::implementation

namespace winrt::kanban::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
};
} // namespace winrt::kanban::factory_implementation
