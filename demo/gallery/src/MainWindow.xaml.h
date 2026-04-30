#pragma once

#include "MainWindow.g.h"
#include "HomePage.xaml.h"
#include "ButtonPage.xaml.h"
#include "TextPage.xaml.h"
#include "SelectionPage.xaml.h"
#include "NavigationPage.xaml.h"
#include "FeedbackPage.xaml.h"
#include "DialogFlyoutPage.xaml.h"
#include "MediaPage.xaml.h"
#include "MenuPage.xaml.h"

namespace winrt::gallery::implementation {
struct MainWindow : MainWindowT<MainWindow> {
    MainWindow();
    void NavView_ItemInvoked(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs const&);
};
} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
} // namespace winrt::gallery::factory_implementation
