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
// Property editor handlers
// ============================================================================

void MenuPage::OpenSwitch_Toggled(wf::IInspectable const&, mux::RoutedEventArgs const&) {
    DemoCommandBar().IsOpen(OpenSwitch().IsOn());
}

void MenuPage::LabelPositionCombo_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    auto pos = muxc::CommandBarDefaultLabelPosition::Right;

    // Map combo index to CommandBarDefaultLabelPosition
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

    // Map combo index to CommandBarOverflowButtonVisibility
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

} // namespace winrt::gallery::implementation
