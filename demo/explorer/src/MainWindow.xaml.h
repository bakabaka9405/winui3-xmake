#pragma once

#include "ExplorerPage.xaml.h"
#include "MainWindow.g.h"

namespace winrt::explorer::implementation {
struct MainWindow : MainWindowT<MainWindow> {
	MainWindow();
	void NavView_ItemInvoked(
		Windows::Foundation::IInspectable const&,
		Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs const&);
};
} // namespace winrt::explorer::implementation

namespace winrt::explorer::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
} // namespace winrt::explorer::factory_implementation
