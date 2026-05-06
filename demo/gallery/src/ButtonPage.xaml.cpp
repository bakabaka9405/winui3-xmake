#include "ButtonPage.xaml.h"
#include "pch.h"

#if __has_include("ButtonPage.g.cpp")
#include "ButtonPage.g.cpp"
#endif

namespace winrt::gallery::implementation {

ButtonPage::ButtonPage() {
    InitializeComponent();
}

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

void ButtonPage::DemoCheckBox_Changed(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    auto checked{ DemoCheckBox().IsChecked() };
    if (!checked) {
        EditorCheckChecked().SelectedIndex(2); // Indeterminate (null)
    } else {
        EditorCheckChecked().SelectedIndex(checked.Value() ? 1 : 0); // Checked/Unchecked
    }
    m_updatingButton = false;
}

void ButtonPage::DemoToggleSwitch_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    EditorSwitchOn().IsOn(DemoToggleSwitch().IsOn());
    m_updatingButton = false;
}

void ButtonPage::DemoToggleButton_Changed(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    auto checked{ DemoToggleButton().IsChecked() };
    EditorToggleChecked().IsOn(checked ? checked.Value() : false);
    m_updatingButton = false;
}

void ButtonPage::EditorBtnContent_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoButton().Content(
        winrt::box_value(EditorBtnContent().Text()));
    m_updatingButton = false;
}

void ButtonPage::EditorBtnWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    double w = EditorBtnWidth().Value();
    DemoButton().Width(w);
    BtnWidthLabel().Text(
        L"Width: " + std::to_wstring(static_cast<int32_t>(w)));
    m_updatingButton = false;
}

void ButtonPage::EditorBtnEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoButton().IsEnabled(EditorBtnEnabled().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorBtnVisibility_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    auto vis = mux::Visibility::Visible;

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
    m_updatingButton = false;
}

void ButtonPage::EditorBtnCornerRadius_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    double r = EditorBtnCornerRadius().Value();
    DemoButton().CornerRadius(mux::CornerRadius{ r, r, r, r });
    BtnCornerRadiusLabel().Text(
        L"CornerRadius: " + winrt::to_hstring(static_cast<int32_t>(r)));
    m_updatingButton = false;
}

void ButtonPage::EditorBtnFontSize_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    double fs = EditorBtnFontSize().Value();
    DemoButton().FontSize(fs);
    BtnFontSizeLabel().Text(
        L"FontSize: " + winrt::to_hstring(static_cast<int32_t>(fs)));
    m_updatingButton = false;
}

void ButtonPage::EditorBtnAlignment_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;

    auto align = mux::HorizontalAlignment::Left;
    switch (EditorBtnAlignment().SelectedIndex()) {
    case 0: align = mux::HorizontalAlignment::Stretch; break;
    case 1: align = mux::HorizontalAlignment::Left;    break;
    case 2: align = mux::HorizontalAlignment::Center;  break;
    case 3: align = mux::HorizontalAlignment::Right;   break;
    default: break;
    }

    DemoButton().HorizontalAlignment(align);
    m_updatingButton = false;
}

void ButtonPage::EditorBtnTooltip_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    auto tipText = EditorBtnTooltip().Text();
    if (tipText.empty()) {
        // Use ToolTipService to set empty tooltip (removes it)
        muxc::ToolTipService::SetToolTip(DemoButton(), nullptr);
    } else {
        muxc::ToolTip tooltip;
        tooltip.Content(winrt::box_value(tipText));
        muxc::ToolTipService::SetToolTip(DemoButton(), tooltip);
    }
    m_updatingButton = false;
}

void ButtonPage::EditorToggleContent_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoToggleButton().Content(
        winrt::box_value(EditorToggleContent().Text()));
    m_updatingButton = false;
}

void ButtonPage::EditorToggleChecked_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoToggleButton().IsChecked(
        EditorToggleChecked().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorToggleEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoToggleButton().IsEnabled(EditorToggleEnabled().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorToggleWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    double w = EditorToggleWidth().Value();
    DemoToggleButton().Width(w);
    TglWidthLabel().Text(
        L"Width: " + std::to_wstring(static_cast<int32_t>(w)));
    m_updatingButton = false;
}

void ButtonPage::EditorToggleHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    double h = EditorToggleHeight().Value();
    DemoToggleButton().Height(h);
    TglHeightLabel().Text(
        L"Height: " + std::to_wstring(static_cast<int32_t>(h)));
    m_updatingButton = false;
}

void ButtonPage::EditorToggleAlignment_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;

    auto align = mux::HorizontalAlignment::Left;
    switch (EditorToggleAlignment().SelectedIndex()) {
    case 0: align = mux::HorizontalAlignment::Stretch; break;
    case 1: align = mux::HorizontalAlignment::Left;    break;
    case 2: align = mux::HorizontalAlignment::Center;  break;
    case 3: align = mux::HorizontalAlignment::Right;   break;
    default: break;
    }

    DemoToggleButton().HorizontalAlignment(align);
    m_updatingButton = false;
}

void ButtonPage::EditorSplitWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoSplitButton().Width(EditorSplitWidth().Value());
    m_updatingButton = false;
}

void ButtonPage::EditorSplitHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoSplitButton().Height(EditorSplitHeight().Value());
    m_updatingButton = false;
}

void ButtonPage::EditorSplitEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoSplitButton().IsEnabled(EditorSplitEnabled().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorDropWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoDropDownButton().Width(EditorDropWidth().Value());
    m_updatingButton = false;
}

void ButtonPage::EditorDropHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoDropDownButton().Height(EditorDropHeight().Value());
    m_updatingButton = false;
}

void ButtonPage::EditorDropEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoDropDownButton().IsEnabled(EditorDropEnabled().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorRepeatWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoRepeatButton().Width(EditorRepeatWidth().Value());
    m_updatingButton = false;
}

void ButtonPage::EditorRepeatHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoRepeatButton().Height(EditorRepeatHeight().Value());
    m_updatingButton = false;
}

void ButtonPage::EditorRepeatEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoRepeatButton().IsEnabled(EditorRepeatEnabled().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorRepeatContent_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoRepeatButton().Content(
        winrt::box_value(EditorRepeatContent().Text()));
    m_updatingButton = false;
}

void ButtonPage::EditorRepeatAlignment_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;

    auto align = mux::HorizontalAlignment::Left;
    switch (EditorRepeatAlignment().SelectedIndex()) {
    case 0: align = mux::HorizontalAlignment::Stretch; break;
    case 1: align = mux::HorizontalAlignment::Left;    break;
    case 2: align = mux::HorizontalAlignment::Center;  break;
    case 3: align = mux::HorizontalAlignment::Right;   break;
    default: break;
    }

    DemoRepeatButton().HorizontalAlignment(align);
    m_updatingButton = false;
}

void ButtonPage::EditorHyperlinkWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoHyperlink().Width(EditorHyperlinkWidth().Value());
    m_updatingButton = false;
}

void ButtonPage::EditorHyperlinkHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoHyperlink().Height(EditorHyperlinkHeight().Value());
    m_updatingButton = false;
}

void ButtonPage::EditorHyperlinkEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoHyperlink().IsEnabled(EditorHyperlinkEnabled().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorCheckContent_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoCheckBox().Content(
        winrt::box_value(EditorCheckContent().Text()));
    m_updatingButton = false;
}

void ButtonPage::EditorCheckChecked_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    switch (EditorCheckChecked().SelectedIndex()) {
    case 0: DemoCheckBox().IsChecked(false); break;
    case 1: DemoCheckBox().IsChecked(true); break;
    case 2: DemoCheckBox().IsChecked(nullptr); break;
    }
    m_updatingButton = false;
}

void ButtonPage::EditorCheckThreeState_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoCheckBox().IsThreeState(
        EditorCheckThreeState().IsOn());
    // When switching to three-state mode, reset to unchecked and sync toggle
    if (EditorCheckThreeState().IsOn()) {
        DemoCheckBox().IsChecked(false);
        EditorCheckChecked().SelectedIndex(0);
    }
    m_updatingButton = false;
}

void ButtonPage::EditorCheckEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoCheckBox().IsEnabled(EditorCheckEnabled().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorCheckWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    double w = EditorCheckWidth().Value();
    DemoCheckBox().Width(w);
    ChkWidthLabel().Text(
        L"Width: " + std::to_wstring(static_cast<int32_t>(w)));
    m_updatingButton = false;
}

void ButtonPage::EditorCheckFontSize_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    double fs = EditorCheckFontSize().Value();
    DemoCheckBox().FontSize(fs);
    ChkFontSizeLabel().Text(
        L"FontSize: " + std::to_wstring(static_cast<int32_t>(fs)));
    m_updatingButton = false;
}

void ButtonPage::EditorRadioWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoRadioGroup().Width(EditorRadioWidth().Value());
    m_updatingButton = false;
}

void ButtonPage::EditorRadioHeight_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoRadioGroup().Height(EditorRadioHeight().Value());
    m_updatingButton = false;
}

void ButtonPage::EditorRadioEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoRadioGroup().IsEnabled(EditorRadioEnabled().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorRadioAlignment_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;

    auto align = mux::HorizontalAlignment::Left;
    switch (EditorRadioAlignment().SelectedIndex()) {
    case 0: align = mux::HorizontalAlignment::Stretch; break;
    case 1: align = mux::HorizontalAlignment::Left;    break;
    case 2: align = mux::HorizontalAlignment::Center;  break;
    case 3: align = mux::HorizontalAlignment::Right;   break;
    default: break;
    }

    DemoRadioGroup().HorizontalAlignment(align);
    m_updatingButton = false;
}

void ButtonPage::EditorSwitchHeader_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoToggleSwitch().Header(
        winrt::box_value(EditorSwitchHeader().Text()));
    m_updatingButton = false;
}

void ButtonPage::EditorSwitchOn_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoToggleSwitch().IsOn(EditorSwitchOn().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorSwitchEnabled_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    DemoToggleSwitch().IsEnabled(EditorSwitchEnabled().IsOn());
    m_updatingButton = false;
}

void ButtonPage::EditorSwitchWidth_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    if (m_updatingButton) return;
    m_updatingButton = true;
    double w = EditorSwitchWidth().Value();
    DemoToggleSwitch().Width(w);
    SwWidthLabel().Text(
        L"Width: " + std::to_wstring(static_cast<int32_t>(w)));
    m_updatingButton = false;
}

} // namespace winrt::gallery::implementation
