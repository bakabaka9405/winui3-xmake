#include "MainWindow.xaml.h"
#include "pch.h"

#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::hello::implementation {
MainWindow::MainWindow() {
	InitializeComponent();

	// auto acrylic = mux::Media::DesktopAcrylicBackdrop();
	// SystemBackdrop(acrylic);

	// auto mica = mux::Media::MicaBackdrop();
	// mica.Kind(muic::SystemBackdrops::MicaKind::BaseAlt);
	// SystemBackdrop(mica);

	this->AppWindow().Resize({ 800, 600 });
}

void MainWindow::myButton_Click(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	myButton().Content(winrt::box_value(L"Clicked"));
}
} // namespace winrt::hello::implementation
