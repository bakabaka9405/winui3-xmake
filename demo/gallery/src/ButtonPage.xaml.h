#pragma once

#include "ButtonPage.g.h"

namespace winrt::gallery::implementation {
struct ButtonPage : ButtonPageT<ButtonPage> {
    ButtonPage();

    // ── Control event handlers ──
    void DemoButton_Click(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void DemoRepeatButton_Click(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    // ── Button property editors ──
    void EditorBtnContent_TextChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

    void EditorBtnWidth_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorBtnEnabled_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void EditorBtnVisibility_SelectionChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    // ── ToggleButton property editors ──
    void EditorToggleContent_TextChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

    void EditorToggleChecked_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    // ── SplitButton property editors ──
    void EditorSplitWidth_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorSplitHeight_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorSplitEnabled_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    // ── DropDownButton property editors ──
    void EditorDropWidth_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorDropHeight_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorDropEnabled_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    // ── RepeatButton property editors ──
    void EditorRepeatWidth_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorRepeatHeight_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorRepeatEnabled_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    // ── HyperlinkButton property editors ──
    void EditorHyperlinkWidth_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorHyperlinkHeight_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorHyperlinkEnabled_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    // ── CheckBox property editors ──
    void EditorCheckContent_TextChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

    void EditorCheckChecked_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void EditorCheckThreeState_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    // ── RadioButtons property editors ──
    void EditorRadioWidth_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorRadioHeight_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void EditorRadioEnabled_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    // ── ToggleSwitch property editors ──
    void EditorSwitchHeader_TextChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

    void EditorSwitchOn_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

private:
    int32_t m_repeatCount{ 0 };
};
} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct ButtonPage : ButtonPageT<ButtonPage, implementation::ButtonPage> {
};
} // namespace winrt::gallery::factory_implementation
