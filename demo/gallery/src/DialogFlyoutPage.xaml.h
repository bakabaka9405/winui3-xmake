#pragma once

#include "DialogFlyoutPage.g.h"

namespace winrt::gallery::implementation {
struct DialogFlyoutPage : DialogFlyoutPageT<DialogFlyoutPage> {
    DialogFlyoutPage();

    //  ContentDialog 
    winrt::fire_and_forget OnShowDialogClick(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    //  Flyout 
    void OnFlyoutOkClick(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void OnFlyoutPlacementChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    void OnFlyoutShowModeChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    void OnFlyoutWidthChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void OnFlyoutLightDismissChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    //  MenuFlyout — placement & item click handlers
    void OnMenuFlyoutPlacementChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    void OnCutClick(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void OnCopyClick(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void OnPasteClick(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    //  TeachingTip 
    void OnShowTeachingTipClick(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void OnTeachingTipLightDismissToggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void OnTeachingTipPlacementChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    void OnTeachingTipTitleChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

    void OnTeachingTipSubtitleChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);
};
} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct DialogFlyoutPage : DialogFlyoutPageT<DialogFlyoutPage, implementation::DialogFlyoutPage> {
};
} // namespace winrt::gallery::factory_implementation
