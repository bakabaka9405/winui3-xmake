#include "NavigationPage.xaml.h"
#include "pch.h"

#if __has_include("NavigationPage.g.cpp")
#include "NavigationPage.g.cpp"
#endif

namespace winrt::gallery::implementation {

NavigationPage::NavigationPage() {
    InitializeComponent();
    m_isInitializing = false;
}

void NavigationPage::DemoTabView_AddTabButtonClick(
    Windows::Foundation::IInspectable const&,
    Windows::Foundation::IInspectable const&) {
    if (m_isInitializing) return;
    auto newTab = muxc::TabViewItem{};
    newTab.Header(winrt::box_value(hstring{L"Tab " + std::to_wstring(m_tabCounter)}));
    auto content = mux::Controls::TextBlock{};
    content.Text(L"Content of Tab " + std::to_wstring(m_tabCounter));
    content.Margin(mux::Thickness{16.0});
    newTab.Content(content);
    newTab.IsClosable(CanCloseTabsSwitch().IsOn());
    DemoTabView().TabItems().Append(newTab);
    ++m_tabCounter;
    UpdateTabConfigLabels();
}

void NavigationPage::DemoTabView_TabCloseRequested(
    Windows::Foundation::IInspectable const&,
    Windows::Foundation::IInspectable const& args) {
    if (m_isInitializing) return;
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
    UpdateTabConfigLabels();
}

void NavigationPage::DemoTabView_SelectionChanged(
    Windows::Foundation::IInspectable const&,
    Windows::Foundation::IInspectable const&) {
    if (m_isInitializing) return;
    int32_t idx = DemoTabView().SelectedIndex();
    TabSelectedIndexSlider().Value(idx);
}

void NavigationPage::CanCloseTabsSwitch_Toggled(
    Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    if (m_isInitializing) return;
    bool canClose = CanCloseTabsSwitch().IsOn();
    for (auto const& item : DemoTabView().TabItems()) {
        item.as<muxc::TabViewItem>().IsClosable(canClose);
    }
}

void NavigationPage::IsAddTabVisibleSwitch_Toggled(
    Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    if (m_isInitializing) return;
    DemoTabView().IsAddTabButtonVisible(IsAddTabVisibleSwitch().IsOn());
}

void NavigationPage::TabWidthModeCombo_SelectionChanged(
    Windows::Foundation::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    if (m_isInitializing) return;
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
    if (m_isInitializing) return;
    if (m_updatingNav) return;
    m_updatingNav = true;

    UpdatePipsPageLabel();

    // Sync the page-index slider and its label
    auto pager = DemoPipsPager();
    int32_t newIdx = pager.SelectedPageIndex() + 1;
    PipsPageIndexSlider().Value(newIdx);
    PipsPageIndexLabel().Text(hstring{L"Selected Page Index: " + std::to_wstring(newIdx)});

    m_updatingNav = false;
}

void NavigationPage::PipsPrevButton_Click(
    Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    if (m_isInitializing) return;
    auto pager = DemoPipsPager();
    int32_t idx = pager.SelectedPageIndex();
    if (idx > 0) {
        pager.SelectedPageIndex(idx - 1);
    }
}

void NavigationPage::PipsNextButton_Click(
    Windows::Foundation::IInspectable const&,
    Microsoft::UI::Xaml::RoutedEventArgs const&) {
    if (m_isInitializing) return;
    auto pager = DemoPipsPager();
    int32_t idx = pager.SelectedPageIndex();
    if (idx < pager.NumberOfPages() - 1) {
        pager.SelectedPageIndex(idx + 1);
    }
}

void NavigationPage::NumPagesSlider_ValueChanged(
    Windows::Foundation::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_isInitializing) return;
    int32_t numPages = static_cast<int32_t>(NumPagesSlider().Value());
    DemoPipsPager().NumberOfPages(numPages);
    NumPagesLabel().Text(hstring{L"Number of Pages: " + std::to_wstring(numPages)});
    PipsPageIndexSlider().Maximum(numPages);
    if (DemoPipsPager().SelectedPageIndex() >= numPages) {
        DemoPipsPager().SelectedPageIndex(numPages - 1);
    }
    UpdatePipsPageLabel();
}

void NavigationPage::MaxVisiblePipsSlider_ValueChanged(
    Windows::Foundation::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_isInitializing) return;
    int32_t maxPips = static_cast<int32_t>(MaxVisiblePipsSlider().Value());
    DemoPipsPager().MaxVisiblePips(maxPips);
    MaxVisibleLabel().Text(hstring{L"Max Visible Pips: " + std::to_wstring(maxPips)});
}

void NavigationPage::PipsOrientationCombo_SelectionChanged(
    Windows::Foundation::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    if (m_isInitializing) return;
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

// --- New: TabView config handlers ---

void NavigationPage::TabStripHeaderBox_TextChanged(
    Windows::Foundation::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    if (m_isInitializing) return;
    DemoTabView().TabStripHeader(
        winrt::box_value(TabStripHeaderBox().Text()));
}

void NavigationPage::TabSelectedIndexSlider_ValueChanged(
    Windows::Foundation::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_isInitializing) return;
    int32_t idx = static_cast<int32_t>(TabSelectedIndexSlider().Value());
    auto items = DemoTabView().TabItems();
    if (static_cast<uint32_t>(idx) < items.Size()) {
        DemoTabView().SelectedIndex(idx);
    }
}

void NavigationPage::PipsPageIndexSlider_ValueChanged(
    Windows::Foundation::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_isInitializing) return;
    if (m_updatingNav) return;
    m_updatingNav = true;

    int32_t pageIdx = static_cast<int32_t>(PipsPageIndexSlider().Value());
    // Slider is 1-based; PipsPager.SelectedPageIndex is 0-based
    DemoPipsPager().SelectedPageIndex(pageIdx - 1);
    UpdatePipsPageLabel();
    PipsPageIndexLabel().Text(hstring{L"Selected Page Index: " + std::to_wstring(pageIdx)});

    m_updatingNav = false;
}

void NavigationPage::UpdateTabConfigLabels() {
    auto items = DemoTabView().TabItems();
    uint32_t count = items.Size();
    TabStripFooterLabel().Text(hstring{L"Tab Count: " + std::to_wstring(count)});

    // Update the tab-selection slider range
    int32_t maxIdx = count > 0 ? static_cast<int32_t>(count) - 1 : 0;
    TabSelectedIndexSlider().Maximum(maxIdx);
}

} // namespace winrt::gallery::implementation
