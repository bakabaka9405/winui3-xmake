#pragma once

#include "FeedbackPage.g.h"

namespace winrt::gallery::implementation {
struct FeedbackPage : FeedbackPageT<FeedbackPage> {
    FeedbackPage();

    //  ProgressBar handlers
    void DemoProgressBar_ValueChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    void ProgressBarValueSlider_ValueChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    void ProgressBarIndeterminateToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void ProgressBarShowPausedToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void ProgressBarShowErrorToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void ProgressBarWidthSlider_ValueChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    void ProgressBarMinSlider_ValueChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    //  ProgressRing handlers
    void ProgressRingActiveToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void ProgressRingWidthSlider_ValueChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    void ProgressRingHeightSlider_ValueChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    void ProgressRingDeterminateToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void ProgressRingValueSlider_ValueChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    //  InfoBar handlers
    void InfoBarAction_Click(
        Windows::Foundation::IInspectable const&,
        Windows::Foundation::IInspectable const&);

    void InfoBarSeverityCombo_SelectionChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);

    void InfoBarTitleBox_TextChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const&);

    void InfoBarMessageBox_TextChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const&);

    void InfoBarClosableToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void InfoBarOpenToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void DemoInfoBar_CloseButtonClick(
        Windows::Foundation::IInspectable const&,
        Windows::Foundation::IInspectable const&);

    void InfoBarIconVisibleToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void InfoBarActionButtonBox_TextChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const&);

    void InfoBarWidthSlider_ValueChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    //  Indeterminate ProgressBar handlers
    void IndeterminateBarWidthSlider_ValueChanged(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const&);

    void IndeterminateBarShowPausedToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

    void IndeterminateBarShowErrorToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

private:
    bool m_updatingFeedback{ false };
    bool m_isInitializing{ true };
};
} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct FeedbackPage : FeedbackPageT<FeedbackPage, implementation::FeedbackPage> {
};
} // namespace winrt::gallery::factory_implementation
