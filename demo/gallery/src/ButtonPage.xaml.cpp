#include "ButtonPage.xaml.h"
#include "pch.h"

#if __has_include("ButtonPage.g.cpp")
#include "ButtonPage.g.cpp"
#endif

namespace winrt::gallery::implementation {

ButtonPage::ButtonPage() {
    InitializeComponent();
}

// ============================================================================
// Control event handlers
// ============================================================================

void ButtonPage::DemoButton_Click(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    StatusText().Text(L"Button clicked!");
}

void ButtonPage::DemoRepeatButton_Click(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    ++m_repeatCount;
    RepeatCountText().Text(
        L"Count: " + std::to_wstring(m_repeatCount));
}

// ============================================================================
// Button property editors
// ============================================================================

void ButtonPage::EditorBtnContent_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    DemoButton().Content(
        winrt::box_value(EditorBtnContent().Text()));
}

void ButtonPage::EditorBtnWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    double w = EditorBtnWidth().Value();
    DemoButton().Width(w);
    BtnWidthLabel().Text(
        L"Width: " + std::to_wstring(static_cast<int32_t>(w)));
}

void ButtonPage::EditorBtnEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoButton().IsEnabled(EditorBtnEnabled().IsOn());
}

void ButtonPage::EditorBtnVisibility_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    auto vis = mux::Visibility::Visible; // default

    switch (EditorBtnVisibility().SelectedIndex()) {
    case 0:
        vis = mux::Visibility::Visible;
        break;
    case 1:
        vis = mux::Visibility::Collapsed;
        break;
    default:
        break;
    }

    DemoButton().Visibility(vis);
}

// ============================================================================
// ToggleButton property editors
// ============================================================================

void ButtonPage::EditorToggleContent_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    DemoToggleButton().Content(
        winrt::box_value(EditorToggleContent().Text()));
}

void ButtonPage::EditorToggleChecked_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoToggleButton().IsChecked(
        EditorToggleChecked().IsOn());
}

// ============================================================================
// SplitButton property editors
// ============================================================================

void ButtonPage::EditorSplitWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    DemoSplitButton().Width(EditorSplitWidth().Value());
}

void ButtonPage::EditorSplitHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    DemoSplitButton().Height(EditorSplitHeight().Value());
}

void ButtonPage::EditorSplitEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoSplitButton().IsEnabled(EditorSplitEnabled().IsOn());
}

// ============================================================================
// DropDownButton property editors
// ============================================================================

void ButtonPage::EditorDropWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    DemoDropDownButton().Width(EditorDropWidth().Value());
}

void ButtonPage::EditorDropHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    DemoDropDownButton().Height(EditorDropHeight().Value());
}

void ButtonPage::EditorDropEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoDropDownButton().IsEnabled(EditorDropEnabled().IsOn());
}

// ============================================================================
// RepeatButton property editors
// ============================================================================

void ButtonPage::EditorRepeatWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    DemoRepeatButton().Width(EditorRepeatWidth().Value());
}

void ButtonPage::EditorRepeatHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    DemoRepeatButton().Height(EditorRepeatHeight().Value());
}

void ButtonPage::EditorRepeatEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoRepeatButton().IsEnabled(EditorRepeatEnabled().IsOn());
}

// ============================================================================
// HyperlinkButton property editors
// ============================================================================

void ButtonPage::EditorHyperlinkWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    DemoHyperlink().Width(EditorHyperlinkWidth().Value());
}

void ButtonPage::EditorHyperlinkHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    DemoHyperlink().Height(EditorHyperlinkHeight().Value());
}

void ButtonPage::EditorHyperlinkEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoHyperlink().IsEnabled(EditorHyperlinkEnabled().IsOn());
}

// ============================================================================
// CheckBox property editors
// ============================================================================

void ButtonPage::EditorCheckContent_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    DemoCheckBox().Content(
        winrt::box_value(EditorCheckContent().Text()));
}

void ButtonPage::EditorCheckChecked_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoCheckBox().IsChecked(
        EditorCheckChecked().IsOn());
}

void ButtonPage::EditorCheckThreeState_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoCheckBox().IsThreeState(
        EditorCheckThreeState().IsOn());
    // Reset to unchecked when switching to three-state mode
    if (EditorCheckThreeState().IsOn() && EditorCheckChecked().IsOn()) {
        EditorCheckChecked().IsOn(false);
    }
}

// ============================================================================
// RadioButtons property editors
// ============================================================================

void ButtonPage::EditorRadioWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    DemoRadioGroup().Width(EditorRadioWidth().Value());
}

void ButtonPage::EditorRadioHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    DemoRadioGroup().Height(EditorRadioHeight().Value());
}

void ButtonPage::EditorRadioEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoRadioGroup().IsEnabled(EditorRadioEnabled().IsOn());
}

// ============================================================================
// ToggleSwitch property editors
// ============================================================================

void ButtonPage::EditorSwitchHeader_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    DemoToggleSwitch().Header(
        winrt::box_value(EditorSwitchHeader().Text()));
}

void ButtonPage::EditorSwitchOn_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoToggleSwitch().IsOn(EditorSwitchOn().IsOn());
}

} // namespace winrt::gallery::implementation
