#include "MainWindow.xaml.h"
#include "pch.h"

#include <microsoft.ui.xaml.window.h>
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::xmake_demo::implementation {
MainWindow::MainWindow() {
	InitializeComponent();

	auto mica = mux::Media::MicaBackdrop();
	mica.Kind(muic::SystemBackdrops::MicaKind::BaseAlt);
	SystemBackdrop(mica);

	auto window = try_as<IWindowNative>();
	if (window) {
		HWND hwnd = nullptr;
		window->get_WindowHandle(&hwnd);
		auto id = mui::GetWindowIdFromWindow(hwnd);
		auto appWindow = muiw::AppWindow::GetFromWindowId(id);
		appWindow.Resize({ 800, 600 });
	}
}

void MainWindow::myButton_Click(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	myButton().Content(winrt::box_value(L"Clicked"));
}
} // namespace winrt::xmake_demo::implementation
