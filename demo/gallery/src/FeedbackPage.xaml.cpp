#include "FeedbackPage.xaml.h"
#include "pch.h"

#if __has_include("FeedbackPage.g.cpp")
#include "FeedbackPage.g.cpp"
#endif

namespace winrt::gallery::implementation {
FeedbackPage::FeedbackPage() {
	InitializeComponent();
}

//  Determinate ProgressBar: Value slider
void FeedbackPage::ProgressBarValueSlider_ValueChanged(
	wf::IInspectable const& sender,
	muxp::RangeBaseValueChangedEventArgs const&) {
	DemoProgressBar().Value(sender.as<muxc::Slider>().Value());
}

//  Determinate ProgressBar: IsIndeterminate toggle
void FeedbackPage::ProgressBarIndeterminateToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	DemoProgressBar().IsIndeterminate(sender.as<muxc::ToggleSwitch>().IsOn());
}

//  Determinate ProgressBar: ShowPaused toggle
void FeedbackPage::ProgressBarShowPausedToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	DemoProgressBar().ShowPaused(sender.as<muxc::ToggleSwitch>().IsOn());
}

//  Determinate ProgressBar: ShowError toggle
void FeedbackPage::ProgressBarShowErrorToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	DemoProgressBar().ShowError(sender.as<muxc::ToggleSwitch>().IsOn());
}

//  ProgressRing: IsActive toggle
void FeedbackPage::ProgressRingActiveToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	DemoProgressRing().IsActive(sender.as<muxc::ToggleSwitch>().IsOn());
}

//  InfoBar: ActionButton click
void FeedbackPage::InfoBarAction_Click(
	wf::IInspectable const&,
	wf::IInspectable const&) {
	DemoInfoBar().Title(L"Action Clicked");
	DemoInfoBar().Message(L"The action button was clicked successfully.");
}

//  InfoBar: Severity combo
void FeedbackPage::InfoBarSeverityCombo_SelectionChanged(
	wf::IInspectable const& sender,
	muxc::SelectionChangedEventArgs const&) {
	auto const index = sender.as<muxc::ComboBox>().SelectedIndex();

	// Map combo index → InfoBarSeverity
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

//  InfoBar: Title text
void FeedbackPage::InfoBarTitleBox_TextChanged(
	wf::IInspectable const& sender,
	muxc::TextChangedEventArgs const&) {
	DemoInfoBar().Title(sender.as<muxc::TextBox>().Text());
}

//  InfoBar: Message text
void FeedbackPage::InfoBarMessageBox_TextChanged(
	wf::IInspectable const& sender,
	muxc::TextChangedEventArgs const&) {
	DemoInfoBar().Message(sender.as<muxc::TextBox>().Text());
}

//  InfoBar: IsClosable toggle
void FeedbackPage::InfoBarClosableToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	DemoInfoBar().IsClosable(sender.as<muxc::ToggleSwitch>().IsOn());
}

//  InfoBar: IsOpen toggle
void FeedbackPage::InfoBarOpenToggle_Toggled(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	DemoInfoBar().IsOpen(sender.as<muxc::ToggleSwitch>().IsOn());
}
} // namespace winrt::gallery::implementation
