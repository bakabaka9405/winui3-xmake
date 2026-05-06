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

    // Secondary button — shown only when text is non-empty
    auto secondaryText = DialogSecondaryBtnText().Text();
    if (!secondaryText.empty()) {
        dialog.SecondaryButtonText(secondaryText);
    }

    // Close button
    auto closeText = DialogCloseBtnText().Text();
    if (!closeText.empty()) {
        dialog.CloseButtonText(closeText);
    }

    // Default button
    switch (DialogDefaultBtnCombo().SelectedIndex()) {
    case 0:
        dialog.DefaultButton(muxc::ContentDialogButton::None);
        break;
    case 1:
        dialog.DefaultButton(muxc::ContentDialogButton::Primary);
        break;
    case 2:
        dialog.DefaultButton(muxc::ContentDialogButton::Secondary);
        break;
    case 3:
        dialog.DefaultButton(muxc::ContentDialogButton::Close);
        break;
    default:
        break;
    }

    dialog.XamlRoot(this->XamlRoot());

    // --- Show and await result ---
    auto result = co_await dialog.ShowAsync();

    // --- Update status based on user choice ---
    switch (result) {
    case muxc::ContentDialogResult::Primary:
        StatusText().Text(L"Confirmed (Primary)");
        break;
    case muxc::ContentDialogResult::Secondary:
        StatusText().Text(L"Confirmed (Secondary)");
        break;
    default:
        StatusText().Text(L"Cancelled");
        break;
    }
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
// Flyout — show mode combo changed
// ============================================================================
void DialogFlyoutPage::OnFlyoutShowModeChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    muxp::FlyoutShowMode mode = muxp::FlyoutShowMode::Standard;

    switch (FlyoutShowModeCombo().SelectedIndex()) {
    case 0:
        mode = muxp::FlyoutShowMode::Standard;
        break;
    case 1:
        mode = muxp::FlyoutShowMode::Transient;
        break;
    case 2:
        mode = muxp::FlyoutShowMode::TransientWithDismissOnPointerMoveAway;
        break;
    default:
        break;
    }

    DemoFlyout().ShowMode(mode);
}

// ============================================================================
// Flyout — width slider for the TextBox inside the flyout
// ============================================================================
void DialogFlyoutPage::OnFlyoutWidthChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    FlyoutTextBox().Width(FlyoutWidthSlider().Value());
}

// ============================================================================
// Flyout — light-dismiss overlay mode combo changed
// ============================================================================
void DialogFlyoutPage::OnFlyoutLightDismissChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    muxc::LightDismissOverlayMode mode = muxc::LightDismissOverlayMode::Auto;

    switch (FlyoutLightDismissCombo().SelectedIndex()) {
    case 0:
        mode = muxc::LightDismissOverlayMode::Auto;
        break;
    case 1:
        mode = muxc::LightDismissOverlayMode::On;
        break;
    case 2:
        mode = muxc::LightDismissOverlayMode::Off;
        break;
    default:
        break;
    }

    DemoFlyout().LightDismissOverlayMode(mode);
}

// ============================================================================
// MenuFlyout — placement combo changed
// ============================================================================
void DialogFlyoutPage::OnMenuFlyoutPlacementChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    muxp::FlyoutPlacementMode mode = muxp::FlyoutPlacementMode::Top;

    switch (MenuFlyoutPlacementCombo().SelectedIndex()) {
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

    DemoMenuFlyout().Placement(mode);
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

// ============================================================================
// TeachingTip — preferred placement combo changed
// ============================================================================
void DialogFlyoutPage::OnTeachingTipPlacementChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
    muxc::TeachingTipPlacementMode mode = muxc::TeachingTipPlacementMode::Auto;

    switch (TeachingTipPlacementCombo().SelectedIndex()) {
    case 0:
        mode = muxc::TeachingTipPlacementMode::Auto;
        break;
    case 1:
        mode = muxc::TeachingTipPlacementMode::Top;
        break;
    case 2:
        mode = muxc::TeachingTipPlacementMode::Bottom;
        break;
    case 3:
        mode = muxc::TeachingTipPlacementMode::Left;
        break;
    case 4:
        mode = muxc::TeachingTipPlacementMode::Right;
        break;
    case 5:
        mode = muxc::TeachingTipPlacementMode::TopLeft;
        break;
    case 6:
        mode = muxc::TeachingTipPlacementMode::TopRight;
        break;
    case 7:
        mode = muxc::TeachingTipPlacementMode::BottomLeft;
        break;
    case 8:
        mode = muxc::TeachingTipPlacementMode::BottomRight;
        break;
    default:
        break;
    }

    DemoTeachingTip().PreferredPlacement(mode);
}

// ============================================================================
// TeachingTip — title text changed
// ============================================================================
void DialogFlyoutPage::OnTeachingTipTitleChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    DemoTeachingTip().Title(TeachingTipTitleBox().Text());
}

// ============================================================================
// TeachingTip — subtitle text changed
// ============================================================================
void DialogFlyoutPage::OnTeachingTipSubtitleChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    DemoTeachingTip().Subtitle(TeachingTipSubtitleBox().Text());
}

} // namespace winrt::gallery::implementation
