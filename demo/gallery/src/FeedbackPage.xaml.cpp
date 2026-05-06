#include "FeedbackPage.xaml.h"
#include "pch.h"

#if __has_include("FeedbackPage.g.cpp")
#include "FeedbackPage.g.cpp"
#endif

namespace winrt::gallery::implementation {
FeedbackPage::FeedbackPage() {
	InitializeComponent();

	DemoProgressBar().Width(ProgressBarWidthSlider().Value());
	DemoProgressRing().Width(ProgressRingWidthSlider().Value());
	DemoProgressRing().Height(ProgressRingHeightSlider().Value());
	DemoIndeterminateBar().Width(IndeterminateBarWidthSlider().Value());
	DemoInfoBar().Width(InfoBarWidthSlider().Value());

	m_isInitializing = false;
}

void FeedbackPage::DemoProgressBar_ValueChanged(
	wf::IInspectable const& sender,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	if (m_updatingFeedback) return;

	double const newValue = sender.as<muxc::ProgressBar>().Value();
	double const sliderValue = ProgressBarValueSlider().Value();
	if (newValue != sliderValue) {
		ProgressBarValueSlider().Value(newValue);
	}
}

void FeedbackPage::ProgressBarValueSlider_ValueChanged(
	wf::IInspectable const& sender,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	m_updatingFeedback = true;
	DemoProgressBar().Value(sender.as<muxc::Slider>().Value());
	m_updatingFeedback = false;
}

void FeedbackPage::ProgressBarIndeterminateToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoProgressBar().IsIndeterminate(sender.as<muxc::ToggleSwitch>().IsOn());
}

void FeedbackPage::ProgressBarShowPausedToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoProgressBar().ShowPaused(sender.as<muxc::ToggleSwitch>().IsOn());
}

void FeedbackPage::ProgressBarShowErrorToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoProgressBar().ShowError(sender.as<muxc::ToggleSwitch>().IsOn());
}

void FeedbackPage::ProgressBarWidthSlider_ValueChanged(
	wf::IInspectable const& sender,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto const val = static_cast<int>(sender.as<muxc::Slider>().Value());
	ProgressBarWidthLabel().Text(winrt::to_hstring(val));
	DemoProgressBar().Width(val);
}

void FeedbackPage::ProgressBarMinSlider_ValueChanged(
	wf::IInspectable const& sender,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto const val = static_cast<int>(sender.as<muxc::Slider>().Value());
	ProgressBarMinLabel().Text(winrt::to_hstring(val));

	// Ensure Value >= new Minimum; reset if needed
	DemoProgressBar().Minimum(val);
	if (DemoProgressBar().Value() < val) {
		DemoProgressBar().Value(val);
		ProgressBarValueSlider().Value(val);
	}
}

void FeedbackPage::ProgressRingActiveToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoProgressRing().IsActive(sender.as<muxc::ToggleSwitch>().IsOn());
}

void FeedbackPage::ProgressRingWidthSlider_ValueChanged(
	wf::IInspectable const& sender,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto const val = static_cast<int>(sender.as<muxc::Slider>().Value());
	ProgressRingWidthLabel().Text(winrt::to_hstring(val));
	DemoProgressRing().Width(val);
}

void FeedbackPage::ProgressRingHeightSlider_ValueChanged(
	wf::IInspectable const& sender,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto const val = static_cast<int>(sender.as<muxc::Slider>().Value());
	ProgressRingHeightLabel().Text(winrt::to_hstring(val));
	DemoProgressRing().Height(val);
}

void FeedbackPage::ProgressRingDeterminateToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	bool const determinate = sender.as<muxc::ToggleSwitch>().IsOn();

	DemoProgressRing().IsIndeterminate(!determinate);
	mux::Visibility vis = determinate
		? mux::Visibility::Visible
		: mux::Visibility::Collapsed;
	ProgressRingValueLabel().Visibility(vis);
	ProgressRingValueSlider().Visibility(vis);

	// Apply current slider value when entering determinate mode
	if (determinate) {
		DemoProgressRing().Value(ProgressRingValueSlider().Value());
	}
	else {
		DemoProgressRing().Value(0);
	}
}

void FeedbackPage::ProgressRingValueSlider_ValueChanged(
	wf::IInspectable const& sender,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	DemoProgressRing().Value(sender.as<muxc::Slider>().Value());
}

void FeedbackPage::InfoBarAction_Click(
	wf::IInspectable const&,
	wf::IInspectable const&) {
	DemoInfoBar().Title(L"Action Clicked");
	DemoInfoBar().Message(L"The action button was clicked successfully.");
	InfoBarTitleBox().Text(L"Action Clicked");
	InfoBarMessageBox().Text(L"The action button was clicked successfully.");
}

void FeedbackPage::InfoBarSeverityCombo_SelectionChanged(
	wf::IInspectable const& sender,
	muxc::SelectionChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto const index = sender.as<muxc::ComboBox>().SelectedIndex();

	switch (index) {
	case 0:
		DemoInfoBar().Severity(muxc::InfoBarSeverity::Informational);
		break;
	case 1:
		DemoInfoBar().Severity(muxc::InfoBarSeverity::Success);
		break;
	case 2:
		DemoInfoBar().Severity(muxc::InfoBarSeverity::Warning);
		break;
	case 3:
		DemoInfoBar().Severity(muxc::InfoBarSeverity::Error);
		break;
    default:
        break;
	}
}

void FeedbackPage::InfoBarTitleBox_TextChanged(
	wf::IInspectable const& sender,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	DemoInfoBar().Title(sender.as<muxc::TextBox>().Text());
}

void FeedbackPage::InfoBarMessageBox_TextChanged(
	wf::IInspectable const& sender,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	DemoInfoBar().Message(sender.as<muxc::TextBox>().Text());
}

void FeedbackPage::InfoBarClosableToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoInfoBar().IsClosable(sender.as<muxc::ToggleSwitch>().IsOn());
}

void FeedbackPage::InfoBarOpenToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoInfoBar().IsOpen(sender.as<muxc::ToggleSwitch>().IsOn());
}

//  InfoBar: CloseButtonClick — sync IsOpen toggle when user closes the bar
void FeedbackPage::DemoInfoBar_CloseButtonClick(
	wf::IInspectable const&,
	wf::IInspectable const&) {
	InfoBarOpenToggle().IsOn(false);
}

void FeedbackPage::InfoBarIconVisibleToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoInfoBar().IsIconVisible(sender.as<muxc::ToggleSwitch>().IsOn());
}

void FeedbackPage::InfoBarActionButtonBox_TextChanged(
	wf::IInspectable const& sender,
	muxc::TextChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto const& button = DemoInfoBar().ActionButton().as<muxc::Button>();
	button.Content(winrt::box_value(sender.as<muxc::TextBox>().Text()));
}

void FeedbackPage::InfoBarWidthSlider_ValueChanged(
	wf::IInspectable const& sender,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto const val = static_cast<int>(sender.as<muxc::Slider>().Value());
	InfoBarWidthLabel().Text(winrt::to_hstring(val));
	DemoInfoBar().Width(val);
}

void FeedbackPage::IndeterminateBarWidthSlider_ValueChanged(
	wf::IInspectable const& sender,
	muxp::RangeBaseValueChangedEventArgs const&) {
	if (m_isInitializing) return;
	auto const val = static_cast<int>(sender.as<muxc::Slider>().Value());
	IndeterminateBarWidthLabel().Text(winrt::to_hstring(val));
	DemoIndeterminateBar().Width(val);
}

void FeedbackPage::IndeterminateBarShowPausedToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoIndeterminateBar().ShowPaused(sender.as<muxc::ToggleSwitch>().IsOn());
}

void FeedbackPage::IndeterminateBarShowErrorToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	if (m_isInitializing) return;
	DemoIndeterminateBar().ShowError(sender.as<muxc::ToggleSwitch>().IsOn());
}

} // namespace winrt::gallery::implementation
