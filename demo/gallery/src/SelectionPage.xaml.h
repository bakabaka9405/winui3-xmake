#pragma once

#include "SelectionPage.g.h"

namespace winrt::gallery::implementation {
struct SelectionPage : SelectionPageT<SelectionPage> {
    SelectionPage();

    void OnComboSelectionChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);

    void OnComboEditableToggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void OnListBoxSelectionChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);

    void OnListBoxSelectionModeChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);

    void OnListBoxWidthChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    void OnListBoxHeightChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    void OnListViewSelectionChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);

    void OnListViewSelectionModeChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);

    void OnListViewItemClickToggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void OnGridViewSelectionChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);

    void OnGridViewSelectionModeChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);

private:
    winrt::hstring FormatSelectedItems(
        Windows::Foundation::Collections::IVector<Windows::Foundation::IInspectable> const& items);
    void UpdateGridViewInfo();
};
} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct SelectionPage : SelectionPageT<SelectionPage, implementation::SelectionPage> {
};
} // namespace winrt::gallery::factory_implementation
