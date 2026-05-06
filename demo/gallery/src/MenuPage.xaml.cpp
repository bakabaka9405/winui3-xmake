#include "MenuPage.xaml.h"
#include "pch.h"

#if __has_include("MenuPage.g.cpp")
#include "MenuPage.g.cpp"
#endif

namespace winrt::gallery::implementation {

MenuPage::MenuPage() {
    InitializeComponent();
}

// ============================================================================
// Menu bar click handlers — File
// ============================================================================

void MenuPage::MenuNew_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"File > New");
}

void MenuPage::MenuOpen_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"File > Open");
}

void MenuPage::MenuSave_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"File > Save");
}

void MenuPage::MenuExit_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"File > Exit");
}

// ============================================================================
// Menu bar click handlers — Edit
// ============================================================================

void MenuPage::MenuUndo_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Edit > Undo");
}

void MenuPage::MenuRedo_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Edit > Redo");
}

void MenuPage::MenuCut_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Edit > Cut");
}

void MenuPage::MenuCopy_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Edit > Copy");
}

void MenuPage::MenuPaste_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Edit > Paste");
}

// ============================================================================
// Menu bar click handlers — Help
// ============================================================================

void MenuPage::MenuAbout_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Help > About");
}

// ============================================================================
// CommandBar app bar button click handlers
// ============================================================================

void MenuPage::AddButton_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Add button clicked");
}

void MenuPage::EditButton_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Edit button clicked");
}

void MenuPage::DeleteButton_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Delete button clicked");
}

void MenuPage::SettingsButton_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Settings button clicked");
}

void MenuPage::ShareButton_Click(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    StatusText().Text(L"Share button clicked");
}

// ============================================================================
// CommandBar bidirectional binding — overflow open/close → sync ToggleSwitch
// ============================================================================

void MenuPage::DemoCommandBar_Opening(wf::IInspectable const&, wf::IInspectable const&) {
    if (m_updatingMenu) return;

    m_updatingMenu = true;
    OpenSwitch().IsOn(true);
    m_updatingMenu = false;
}

void MenuPage::DemoCommandBar_Closed(wf::IInspectable const&, wf::IInspectable const&) {
    if (m_updatingMenu) return;

    m_updatingMenu = true;
    OpenSwitch().IsOn(false);
    m_updatingMenu = false;
}

// ============================================================================
// Property editor handlers — CommandBar
// ============================================================================

void MenuPage::OpenSwitch_Toggled(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    if (m_updatingMenu) return;

    m_updatingMenu = true;
    DemoCommandBar().IsOpen(OpenSwitch().IsOn());
    m_updatingMenu = false;
}

void MenuPage::LabelPositionCombo_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    auto pos = muxc::CommandBarDefaultLabelPosition::Right;

    switch (LabelPositionCombo().SelectedIndex()) {
    case 0:
        pos = muxc::CommandBarDefaultLabelPosition::Right;
        break;
    case 1:
        pos = muxc::CommandBarDefaultLabelPosition::Bottom;
        break;
    case 2:
        pos = muxc::CommandBarDefaultLabelPosition::Collapsed;
        break;
    default:
        break;
    }

    DemoCommandBar().DefaultLabelPosition(pos);
}

void MenuPage::OverflowVisibilityCombo_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    auto vis = muxc::CommandBarOverflowButtonVisibility::Auto;

    switch (OverflowVisibilityCombo().SelectedIndex()) {
    case 0:
        vis = muxc::CommandBarOverflowButtonVisibility::Auto;
        break;
    case 1:
        vis = muxc::CommandBarOverflowButtonVisibility::Visible;
        break;
    case 2:
        vis = muxc::CommandBarOverflowButtonVisibility::Collapsed;
        break;
    default:
        break;
    }

    DemoCommandBar().OverflowButtonVisibility(vis);
}

void MenuPage::StickySwitch_Toggled(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    DemoCommandBar().IsSticky(StickySwitch().IsOn());
}

void MenuPage::ClosedDisplayModeCombo_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    auto mode = muxc::AppBarClosedDisplayMode::Compact;

    switch (ClosedDisplayModeCombo().SelectedIndex()) {
    case 0:
        mode = muxc::AppBarClosedDisplayMode::Compact;
        break;
    case 1:
        mode = muxc::AppBarClosedDisplayMode::Minimal;
        break;
    case 2:
        mode = muxc::AppBarClosedDisplayMode::Hidden;
        break;
    default:
        break;
    }

    DemoCommandBar().ClosedDisplayMode(mode);
}

void MenuPage::CmdBarWidthSlider_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const& args) {
    auto const w = static_cast<double>(args.NewValue());

    CmdBarWidthLabel().Text(winrt::to_hstring(static_cast<int>(w)));
    DemoCommandBar().Width(w);
}

// ============================================================================
// Property editor handlers — MenuBar
// ============================================================================

void MenuPage::MenuBarEnabledSwitch_Toggled(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    DemoMenuBar().IsEnabled(MenuBarEnabledSwitch().IsOn());
}

void MenuPage::MenuBarFlowDirectionCombo_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    auto dir = mux::FlowDirection::LeftToRight;

    switch (MenuBarFlowDirectionCombo().SelectedIndex()) {
    case 0:
        dir = mux::FlowDirection::LeftToRight;
        break;
    case 1:
        dir = mux::FlowDirection::RightToLeft;
        break;
    default:
        break;
    }

    DemoMenuBar().FlowDirection(dir);
}

void MenuPage::MenuBarWidthSlider_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const& args) {
    auto const w = static_cast<double>(args.NewValue());

    MenuBarWidthLabel().Text(winrt::to_hstring(static_cast<int>(w)));
    DemoMenuBar().Width(w);
}

} // namespace winrt::gallery::implementation
