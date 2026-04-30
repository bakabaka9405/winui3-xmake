#include "NavigationPage.xaml.h"
#include "pch.h"

#if __has_include("NavigationPage.g.cpp")
#include "NavigationPage.g.cpp"
#endif

namespace winrt::gallery::implementation {

NavigationPage::NavigationPage() {
    InitializeComponent();
}

void NavigationPage::DemoTabView_AddTabButtonClick(
    Windows::Foundation::IInspectable const&,
    Windows::Foundation::IInspectable const&) {
    auto newTab = muxc::TabViewItem{};
    newTab.Header(winrt::box_value(hstring{L"Tab " + std::to_wstring(m_tabCounter)}));
    auto content = mux::Controls::TextBlock{};
    content.Text(L"Content of Tab " + std::to_wstring(m_tabCounter));
    content.Margin(mux::Thickness{16.0});
    newTab.Content(content);
    DemoTabView().TabItems().Append(newTab);
    ++m_tabCounter;
}

void NavigationPage::DemoTabView_TabCloseRequested(
    Windows::Foundation::IInspectable const&,
    Windows::Foundation::IInspectable const& args) {
    // Remove the closing tab from the TabView
    auto eventArgs = args.as<muxc::TabViewTabCloseRequestedEventArgs>();
    auto closingTab = eventArgs.Tab();
    auto items = DemoTabView().TabItems();
    for (uint32_t i = 0; i < items.Size(); ++i) {
        if (items.GetAt(i) == closingTab) {
            items.RemoveAt(i);
            break;
        }
    }
}

void NavigationPage::CanCloseTabsSwitch_Toggled(
    Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    // Note: CanCloseTabs property is not exposed in this C++/WinRT projection.
    // Individual TabViewItems use IsClosable instead — all tabs are closable by default.
    (void)CanCloseTabsSwitch();
}

void NavigationPage::IsAddTabVisibleSwitch_Toggled(
    Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    DemoTabView().IsAddTabButtonVisible(IsAddTabVisibleSwitch().IsOn());
}

void NavigationPage::TabWidthModeCombo_SelectionChanged(
    Windows::Foundation::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    auto mode = muxc::TabViewWidthMode::Equal;
    switch (TabWidthModeCombo().SelectedIndex()) {
    case 0: mode = muxc::TabViewWidthMode::Equal; break;
    case 1: mode = muxc::TabViewWidthMode::SizeToContent; break;
    case 2: mode = muxc::TabViewWidthMode::Compact; break;
    }
    DemoTabView().TabWidthMode(mode);
}

void NavigationPage::DemoPipsPager_SelectedIndexChanged(
    Windows::Foundation::IInspectable const&,
    Windows::Foundation::IInspectable const&) {
    UpdatePipsPageLabel();
}

void NavigationPage::PipsPrevButton_Click(
    Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    auto pager = DemoPipsPager();
    int32_t idx = pager.SelectedPageIndex();
    if (idx > 0) {
        pager.SelectedPageIndex(idx - 1);
    }
}

void NavigationPage::PipsNextButton_Click(
    Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    auto pager = DemoPipsPager();
    int32_t idx = pager.SelectedPageIndex();
    if (idx < pager.NumberOfPages() - 1) {
        pager.SelectedPageIndex(idx + 1);
    }
}

void NavigationPage::NumPagesSlider_ValueChanged(
    Windows::Foundation::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    int32_t numPages = static_cast<int32_t>(NumPagesSlider().Value());
    DemoPipsPager().NumberOfPages(numPages);
    NumPagesLabel().Text(hstring{L"Number of Pages: " + std::to_wstring(numPages)});
    if (DemoPipsPager().SelectedPageIndex() >= numPages) {
        DemoPipsPager().SelectedPageIndex(numPages - 1);
    }
}

void NavigationPage::MaxVisiblePipsSlider_ValueChanged(
    Windows::Foundation::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    int32_t maxPips = static_cast<int32_t>(MaxVisiblePipsSlider().Value());
    DemoPipsPager().MaxVisiblePips(maxPips);
    MaxVisibleLabel().Text(hstring{L"Max Visible Pips: " + std::to_wstring(maxPips)});
}

void NavigationPage::PipsOrientationCombo_SelectionChanged(
    Windows::Foundation::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    auto orientation = muxc::Orientation::Horizontal;
    switch (PipsOrientationCombo().SelectedIndex()) {
    case 0: orientation = muxc::Orientation::Horizontal; break;
    case 1: orientation = muxc::Orientation::Vertical; break;
    }
    DemoPipsPager().Orientation(orientation);
}

void NavigationPage::UpdatePipsPageLabel() {
    auto pager = DemoPipsPager();
    int32_t page = pager.SelectedPageIndex() + 1;
    int32_t total = pager.NumberOfPages();
    PipsPageLabel().Text(hstring{L"Page " + std::to_wstring(page) + L" of " + std::to_wstring(total)});
}

} // namespace winrt::gallery::implementation
