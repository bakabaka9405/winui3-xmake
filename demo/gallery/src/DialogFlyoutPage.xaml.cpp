#include "DialogFlyoutPage.xaml.h"
#include "pch.h"

#if __has_include("DialogFlyoutPage.g.cpp")
#include "DialogFlyoutPage.g.cpp"
#endif

namespace winrt::gallery::implementation {

DialogFlyoutPage::DialogFlyoutPage() {
    InitializeComponent();
}

// ============================================================================
// ContentDialog — async display with property-editor values
// ============================================================================
winrt::fire_and_forget DialogFlyoutPage::OnShowDialogClick(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    // --- Build dialog from property editors ---
    muxc::ContentDialog dialog;
    dialog.Title(winrt::box_value(DialogTitleBox().Text()));
    dialog.Content(winrt::box_value(DialogContentBox().Text()));
    dialog.PrimaryButtonText(DialogPrimaryBtnText().Text());
    dialog.CloseButtonText(L"Cancel");
    dialog.XamlRoot(this->XamlRoot());

    // --- Show and await result ---
    auto result = co_await dialog.ShowAsync();

    // --- Update status based on user choice ---
    StatusText().Text(
        result == muxc::ContentDialogResult::Primary ? L"Confirmed" : L"Cancelled");
}

// ============================================================================
// Flyout — OK button inside the Flyout
// ============================================================================
void DialogFlyoutPage::OnFlyoutOkClick(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    StatusText().Text(L"Flyout input: " + FlyoutTextBox().Text());

    // Dismiss the flyout
    DemoFlyout().Hide();
}

// ============================================================================
// Flyout — placement combo changed
// ============================================================================
void DialogFlyoutPage::OnFlyoutPlacementChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    // Map combo index to FlyoutPlacementMode
    muxp::FlyoutPlacementMode mode = muxp::FlyoutPlacementMode::Top;

    switch (FlyoutPlacementCombo().SelectedIndex()) {
    case 0:
        mode = muxp::FlyoutPlacementMode::Top;
        break;
    case 1:
        mode = muxp::FlyoutPlacementMode::Bottom;
        break;
    case 2:
        mode = muxp::FlyoutPlacementMode::Left;
        break;
    case 3:
        mode = muxp::FlyoutPlacementMode::Right;
        break;
    case 4:
        mode = muxp::FlyoutPlacementMode::Full;
        break;
    default:
        break;
    }

    DemoFlyout().Placement(mode);
}

// ============================================================================
// MenuFlyout — Cut
// ============================================================================
void DialogFlyoutPage::OnCutClick(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    StatusText().Text(L"Cut selected");
}

// ============================================================================
// MenuFlyout — Copy
// ============================================================================
void DialogFlyoutPage::OnCopyClick(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    StatusText().Text(L"Copy selected");
}

// ============================================================================
// MenuFlyout — Paste
// ============================================================================
void DialogFlyoutPage::OnPasteClick(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    StatusText().Text(L"Paste selected");
}

// ============================================================================
// TeachingTip — show button
// ============================================================================
void DialogFlyoutPage::OnShowTeachingTipClick(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoTeachingTip().IsOpen(true);
}

// ============================================================================
// TeachingTip — IsLightDismissEnabled toggle
// ============================================================================
void DialogFlyoutPage::OnTeachingTipLightDismissToggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    DemoTeachingTip().IsLightDismissEnabled(
        TeachingTipLightDismissSwitch().IsOn());
}

} // namespace winrt::gallery::implementation
