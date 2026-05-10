#include "MainWindow.xaml.h"
#include "pch.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::explorer::implementation {

MainWindow::MainWindow() {
	InitializeComponent();
	this->AppWindow().Resize({ 1280, 800 });
	this->Title(L"Explorer");
	auto page = winrt::make<implementation::ExplorerPage>();
	ContentFrame().Content(page);
	auto udp = winrt::Windows::Storage::UserDataPaths::GetDefault();
	page.NavigateTo(udp.Profile());
}

void MainWindow::NavView_ItemInvoked(
	wf::IInspectable const&,
	muxc::NavigationViewItemInvokedEventArgs const& args) {
	auto tag = args.InvokedItemContainer().Tag().as<hstring>();
	auto udp = winrt::Windows::Storage::UserDataPaths::GetDefault();
	hstring path;
	if (tag == L"Desktop") path = udp.Desktop();
	else if (tag == L"Documents") path = udp.Documents();
	else if (tag == L"Downloads") path = udp.Downloads();
	else if (tag == L"Pictures") path = udp.Pictures();
	else if (tag == L"ThisPC") path = L"";

	auto page = ContentFrame().Content().try_as<explorer::ExplorerPage>();
	if (page) page.NavigateTo(path);
}

} // namespace winrt::explorer::implementation
