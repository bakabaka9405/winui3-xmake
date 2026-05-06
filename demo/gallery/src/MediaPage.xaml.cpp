#include "MediaPage.xaml.h"
#include "pch.h"

#if __has_include("MediaPage.g.cpp")
#include "MediaPage.g.cpp"
#endif

#include <cstdio>

namespace winrt::gallery::implementation {

MediaPage::MediaPage() {
	InitializeComponent();

	InitialsBox().Text(DemoPersonPicture().Initials());

	m_isInitializing = false;
}

// ============================================================================
// Image property editors
// ============================================================================
void MediaPage::ImageWidthSlider_ValueChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	double value = ImageWidthSlider().Value();

	DemoImage().Width(value);

	ImageWidthLabel().Text(
		hstring{ L"Width: " + std::to_wstring(static_cast<int32_t>(value)) });
}

void MediaPage::ImageHeightSlider_ValueChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	double value = ImageHeightSlider().Value();

	DemoImage().Height(value);

	ImageHeightLabel().Text(
		hstring{ L"Height: " + std::to_wstring(static_cast<int32_t>(value)) });
}

void MediaPage::ImageStretchCombo_SelectionChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto stretch = mux::Media::Stretch::Uniform; // default

	// Map combo index to Stretch enum
	switch (ImageStretchCombo().SelectedIndex()) {
	case 0:
		stretch = mux::Media::Stretch::None;
		break;
	case 1:
		stretch = mux::Media::Stretch::Fill;
		break;
	case 2:
		stretch = mux::Media::Stretch::Uniform;
		break;
	case 3:
		stretch = mux::Media::Stretch::UniformToFill;
		break;
	default:
		break;
	}

	DemoImage().Stretch(stretch);
}

void MediaPage::DisplayNameBox_TextChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	hstring name = DisplayNameBox().Text();

	DemoPersonPicture().DisplayName(name);
	DemoPersonPicProfile().DisplayName(name);

	InitialsBox().Text(DemoPersonPicture().Initials());
}

void MediaPage::PersonWidthSlider_ValueChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	double value = PersonWidthSlider().Value();

	DemoPersonPicture().Width(value);
	DemoPersonPicProfile().Width(value);

	PersonWidthLabel().Text(
		hstring{ L"Width: " + std::to_wstring(static_cast<int32_t>(value)) });
}

void MediaPage::PersonHeightSlider_ValueChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	double value = PersonHeightSlider().Value();

	DemoPersonPicture().Height(value);
	DemoPersonPicProfile().Height(value);

	PersonHeightLabel().Text(
		hstring{ L"Height: " + std::to_wstring(static_cast<int32_t>(value)) });
}

void MediaPage::IsGroupSwitch_Toggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	bool isGroup = IsGroupSwitch().IsOn();

	DemoPersonPicture().IsGroup(isGroup);
	DemoPersonPicProfile().IsGroup(isGroup);
}

void MediaPage::ImageOpacitySlider_ValueChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	double value = ImageOpacitySlider().Value();

	DemoImage().Opacity(value);

	wchar_t buf[32];
	swprintf_s(buf, L"Opacity: %.1f", value);
	ImageOpacityLabel().Text(hstring{ buf });
}

void MediaPage::ImageCornerRadiusSlider_ValueChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	double value = ImageCornerRadiusSlider().Value();

	DemoImageBorder().CornerRadius(mux::CornerRadius{ value, value, value, value });

	ImageCornerRadiusLabel().Text(
		hstring{ L"CornerRadius: " + std::to_wstring(static_cast<int32_t>(value)) });
}

void MediaPage::BadgeNumberSlider_ValueChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	double value = BadgeNumberSlider().Value();
	int32_t badgeNum = static_cast<int32_t>(value);

	DemoPersonPicture().BadgeNumber(badgeNum);
	DemoPersonPicProfile().BadgeNumber(badgeNum);

	BadgeNumberLabel().Text(
		hstring{ L"BadgeNumber: " + std::to_wstring(badgeNum) });
}

void MediaPage::BadgeGlyphBox_TextChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	hstring glyph = BadgeGlyphBox().Text();

	DemoPersonPicture().BadgeGlyph(glyph);
	DemoPersonPicProfile().BadgeGlyph(glyph);
}

} // namespace winrt::gallery::implementation
