#include "TextPage.xaml.h"
#include "pch.h"

#if __has_include("TextPage.g.cpp")
#include "TextPage.g.cpp"
#endif

namespace winrt::gallery::implementation {

TextPage::TextPage() {
	InitializeComponent();

	// --- Populate AutoSuggestBox suggestion items ---
	auto items = winrt::single_threaded_observable_vector<hstring>();
	items.Append(L"Apple");
	items.Append(L"Banana");
	items.Append(L"Cherry");
	items.Append(L"Date");
	items.Append(L"Fig");
	items.Append(L"Grape");
	items.Append(L"Orange");
	DemoAutoSuggestBox().ItemsSource(items);
}

// ============================================================================
// TextBox property editors
// ============================================================================

void TextPage::OnTextBoxTextChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	DemoTextBox().Text(TextBoxTextEditor().Text());
}

void TextPage::OnTextBoxWidthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	DemoTextBox().Width(TextBoxWidthSlider().Value());
}

void TextPage::OnTextBoxReadOnlyToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoTextBox().IsReadOnly(TextBoxReadOnlyToggle().IsOn());
}

void TextPage::OnTextBoxHeaderChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	DemoTextBox().Header(winrt::box_value(TextBoxHeaderEditor().Text()));
}

// ============================================================================
// PasswordBox property editors
// ============================================================================

void TextPage::OnPasswordRevealModeChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	auto mode = muxc::PasswordRevealMode::Peek; // default

	switch (PasswordRevealModeCombo().SelectedIndex()) {
	case 0: mode = muxc::PasswordRevealMode::Peek; break;
	case 1: mode = muxc::PasswordRevealMode::Hidden; break;
	case 2: mode = muxc::PasswordRevealMode::Visible; break;
	default: break;
	}

	DemoPasswordBox().PasswordRevealMode(mode);
}

void TextPage::OnPasswordRevealButtonToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoPasswordBox().IsPasswordRevealButtonEnabled(PasswordRevealButtonToggle().IsOn());
}

// ============================================================================
// NumberBox property editors
// ============================================================================

void TextPage::OnNumberBoxValueChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	DemoNumberBox().Value(NumberBoxValueSlider().Value());
}

void TextPage::OnSpinButtonModeChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	auto mode = muxc::NumberBoxSpinButtonPlacementMode::Inline; // default

	switch (SpinButtonModeCombo().SelectedIndex()) {
	case 0: mode = muxc::NumberBoxSpinButtonPlacementMode::Inline; break;
	case 1: mode = muxc::NumberBoxSpinButtonPlacementMode::Compact; break;
	case 2: mode = muxc::NumberBoxSpinButtonPlacementMode::Hidden; break;
	default: break;
	}

	DemoNumberBox().SpinButtonPlacementMode(mode);
}

// ============================================================================
// RichEditBox property editors
// ============================================================================

void TextPage::OnRichEditReadOnlyToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoRichEditBox().IsReadOnly(RichEditReadOnlyToggle().IsOn());
}

void TextPage::OnRichEditSpellCheckToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoRichEditBox().IsSpellCheckEnabled(RichEditSpellCheckToggle().IsOn());
}

// ============================================================================
// AutoSuggestBox property editor
// ============================================================================

void TextPage::OnAutoSuggestUpdateToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoAutoSuggestBox().UpdateTextOnSelect(AutoSuggestUpdateToggle().IsOn());
}

} // namespace winrt::gallery::implementation
