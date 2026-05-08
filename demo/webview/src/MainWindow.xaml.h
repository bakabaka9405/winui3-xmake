#pragma once

#include "MainWindow.g.h"
#include <winrt/Microsoft.Web.WebView2.Core.h>

namespace winrt::webview::implementation {
struct MainWindow : MainWindowT<MainWindow> {
	MainWindow();
	winrt::Windows::Foundation::IAsyncAction InitializeWebView();
};
} // namespace winrt::webview::implementation

namespace winrt::webview::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
} // namespace winrt::webview::factory_implementation