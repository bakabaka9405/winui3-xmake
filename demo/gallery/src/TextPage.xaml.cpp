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

	m_isInitializing = false;
}

// ============================================================================
// Reverse binding handlers
// ============================================================================

void TextPage::DemoTextBox_TextChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	if (m_updatingText) return;
	m_updatingText = true;
	TextBoxTextEditor().Text(DemoTextBox().Text());
	m_updatingText = false;
}

void TextPage::DemoNumberBox_ValueChanged(
	wf::IInspectable const&,
	muxc::NumberBoxValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	if (m_updatingNumber) return;
	m_updatingNumber = true;
	NumberBoxValueSlider().Value(DemoNumberBox().Value());
	m_updatingNumber = false;
}

// ============================================================================
// TextBox property editors
// ============================================================================

void TextPage::OnTextBoxTextChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	if (m_updatingText) return;
	m_updatingText = true;
	DemoTextBox().Text(TextBoxTextEditor().Text());
	m_updatingText = false;
}

void TextPage::OnTextBoxWidthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	DemoTextBox().Width(TextBoxWidthSlider().Value());
}

void TextPage::OnTextBoxReadOnlyToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoTextBox().IsReadOnly(TextBoxReadOnlyToggle().IsOn());
}

void TextPage::OnTextBoxHeaderChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	DemoTextBox().Header(winrt::box_value(TextBoxHeaderEditor().Text()));
}

void TextPage::OnTextBoxPlaceholderChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	DemoTextBox().PlaceholderText(TextBoxPlaceholderEditor().Text());
}

void TextPage::OnTextBoxMaxLengthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto val = static_cast<int32_t>(TextBoxMaxLengthSlider().Value());

	TextBoxMaxLengthLabel().Text(winrt::to_hstring(static_cast<int>(val)));
	DemoTextBox().MaxLength(val);
}

void TextPage::OnTextBoxFontSizeChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto val = TextBoxFontSizeSlider().Value();

	TextBoxFontSizeLabel().Text(winrt::to_hstring(static_cast<int>(val)));
	DemoTextBox().FontSize(val);
}

void TextPage::OnTextBoxTextAlignmentChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto alignment = mux::TextAlignment::Left; // default

	switch (TextBoxTextAlignmentCombo().SelectedIndex()) {
	case 0: alignment = mux::TextAlignment::Left; break;
	case 1: alignment = mux::TextAlignment::Center; break;
	case 2: alignment = mux::TextAlignment::Right; break;
	case 3: alignment = mux::TextAlignment::Justify; break;
	default: break;
	}

	DemoTextBox().TextAlignment(alignment);
}

// ============================================================================
// PasswordBox property editors
// ============================================================================

void TextPage::OnPasswordRevealModeChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	if (m_isInitializing) return;
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
	if (m_isInitializing) return;
	DemoPasswordBox().IsPasswordRevealButtonEnabled(PasswordRevealButtonToggle().IsOn());
}

void TextPage::OnPasswordBoxWidthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto val = PasswordBoxWidthSlider().Value();

	PasswordBoxWidthLabel().Text(winrt::to_hstring(static_cast<int>(val)));
	DemoPasswordBox().Width(val);
}

void TextPage::OnPasswordBoxPlaceholderChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	DemoPasswordBox().PlaceholderText(PasswordBoxPlaceholderEditor().Text());
}

// ============================================================================
// NumberBox property editors
// ============================================================================

void TextPage::OnNumberBoxValueChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	if (m_updatingNumber) return;
	m_updatingNumber = true;
	DemoNumberBox().Value(NumberBoxValueSlider().Value());
	m_updatingNumber = false;
}

void TextPage::OnSpinButtonModeChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto mode = muxc::NumberBoxSpinButtonPlacementMode::Inline; // default

	switch (SpinButtonModeCombo().SelectedIndex()) {
	case 0: mode = muxc::NumberBoxSpinButtonPlacementMode::Inline; break;
	case 1: mode = muxc::NumberBoxSpinButtonPlacementMode::Compact; break;
	case 2: mode = muxc::NumberBoxSpinButtonPlacementMode::Hidden; break;
	default: break;
	}

	DemoNumberBox().SpinButtonPlacementMode(mode);
}

void TextPage::OnNumberBoxMinChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto val = NumberBoxMinSlider().Value();

	NumberBoxMinLabel().Text(winrt::to_hstring(static_cast<int>(val)));
	DemoNumberBox().Minimum(val);
}

void TextPage::OnNumberBoxMaxChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto val = NumberBoxMaxSlider().Value();

	NumberBoxMaxLabel().Text(winrt::to_hstring(static_cast<int>(val)));
	DemoNumberBox().Maximum(val);
}

void TextPage::OnNumberBoxWidthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto val = NumberBoxWidthSlider().Value();

	NumberBoxWidthLabel().Text(winrt::to_hstring(static_cast<int>(val)));
	DemoNumberBox().Width(val);
}

// ============================================================================
// RichEditBox property editors
// ============================================================================

void TextPage::OnRichEditReadOnlyToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoRichEditBox().IsReadOnly(RichEditReadOnlyToggle().IsOn());
}

void TextPage::OnRichEditSpellCheckToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoRichEditBox().IsSpellCheckEnabled(RichEditSpellCheckToggle().IsOn());
}

void TextPage::OnRichEditHeightChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto val = RichEditHeightSlider().Value();

	RichEditHeightLabel().Text(winrt::to_hstring(static_cast<int>(val)));
	DemoRichEditBox().Height(val);
}

void TextPage::OnRichEditWidthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto val = RichEditWidthSlider().Value();

	RichEditWidthLabel().Text(winrt::to_hstring(static_cast<int>(val)));
	DemoRichEditBox().Width(val);
}

// ============================================================================
// AutoSuggestBox property editors
// ============================================================================

void TextPage::OnAutoSuggestUpdateToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoAutoSuggestBox().UpdateTextOnSelect(AutoSuggestUpdateToggle().IsOn());
}

void TextPage::OnAutoSuggestPlaceholderChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	DemoAutoSuggestBox().PlaceholderText(AutoSuggestPlaceholderEditor().Text());
}

void TextPage::OnAutoSuggestWidthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto val = AutoSuggestWidthSlider().Value();

	AutoSuggestWidthLabel().Text(winrt::to_hstring(static_cast<int>(val)));
	DemoAutoSuggestBox().Width(val);
}

} // namespace winrt::gallery::implementation
