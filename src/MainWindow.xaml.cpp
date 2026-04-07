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

	auto mica = winrt::Microsoft::UI::Xaml::Media::MicaBackdrop();
	mica.Kind(winrt::Microsoft::UI::Composition::SystemBackdrops::MicaKind::BaseAlt);
	SystemBackdrop(mica);

	auto window = try_as<IWindowNative>();
	if (window) {
		HWND hwnd = nullptr;
		window->get_WindowHandle(&hwnd);
		auto id = Microsoft::UI::GetWindowIdFromWindow(hwnd);
		auto appWindow = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(id);
		appWindow.Resize({ 800, 600 });
	}
}

void MainWindow::myButton_Click(
	Windows::Foundation::IInspectable const&,
	Microsoft::UI::Xaml::RoutedEventArgs const&) {
	myButton().Content(winrt::box_value(L"Clicked"));
}
} // namespace winrt::xmake_demo::implementation
