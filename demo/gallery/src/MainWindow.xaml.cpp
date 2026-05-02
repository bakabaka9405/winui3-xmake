#include "MainWindow.xaml.h"
#include "pch.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::gallery::implementation {
MainWindow::MainWindow() {
    InitializeComponent();
    this->AppWindow().Resize({ 1600, 900 });
    this->Title(L"WinUI 3 Gallery");
    ContentFrame().Navigate(winrt::xaml_typename<gallery::HomePage>());
}

void MainWindow::NavView_ItemInvoked(
    wf::IInspectable const&,
    muxc::NavigationViewItemInvokedEventArgs const& args) {
    auto tag = args.InvokedItemContainer().Tag().as<hstring>();
    if (tag == L"Home")       ContentFrame().Navigate(winrt::xaml_typename<gallery::HomePage>());
    else if (tag == L"Buttons")    ContentFrame().Navigate(winrt::xaml_typename<gallery::ButtonPage>());
    else if (tag == L"Text")       ContentFrame().Navigate(winrt::xaml_typename<gallery::TextPage>());
    else if (tag == L"Selection")  ContentFrame().Navigate(winrt::xaml_typename<gallery::SelectionPage>());
    else if (tag == L"Navigation") ContentFrame().Navigate(winrt::xaml_typename<gallery::NavigationPage>());
    else if (tag == L"Feedback")   ContentFrame().Navigate(winrt::xaml_typename<gallery::FeedbackPage>());
    else if (tag == L"Dialogs")    ContentFrame().Navigate(winrt::xaml_typename<gallery::DialogFlyoutPage>());
    else if (tag == L"Media")      ContentFrame().Navigate(winrt::xaml_typename<gallery::MediaPage>());
    else if (tag == L"Menus")      ContentFrame().Navigate(winrt::xaml_typename<gallery::MenuPage>());
}
} // namespace winrt::gallery::implementation
