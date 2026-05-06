#pragma once

#include "NavigationPage.g.h"

namespace winrt::gallery::implementation {
struct NavigationPage : NavigationPageT<NavigationPage> {
    NavigationPage();

    void DemoTabView_AddTabButtonClick(
        Windows::Foundation::IInspectable const& sender,
        Windows::Foundation::IInspectable const& args);

    void DemoTabView_TabCloseRequested(
        Windows::Foundation::IInspectable const& sender,
        Windows::Foundation::IInspectable const& args);

    void DemoTabView_SelectionChanged(
        Windows::Foundation::IInspectable const& sender,
        Windows::Foundation::IInspectable const& args);

    void CanCloseTabsSwitch_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void IsAddTabVisibleSwitch_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void TabWidthModeCombo_SelectionChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    void DemoPipsPager_SelectedIndexChanged(
        Windows::Foundation::IInspectable const& sender,
        Windows::Foundation::IInspectable const& args);

    void PipsPrevButton_Click(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void PipsNextButton_Click(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void NumPagesSlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void MaxVisiblePipsSlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void PipsOrientationCombo_SelectionChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    // --- New: TabView config handlers ---

    void TabStripHeaderBox_TextChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

    void TabSelectedIndexSlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    // --- New: PipsPager config handlers ---

    void PipsPageIndexSlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

private:
    void UpdatePipsPageLabel();
    void UpdateTabConfigLabels();

    int32_t m_tabCounter{ 4 };
    bool m_updatingNav{ false };
    bool m_isInitializing{ true };
};
} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct NavigationPage : NavigationPageT<NavigationPage, implementation::NavigationPage> {
};
} // namespace winrt::gallery::factory_implementation
