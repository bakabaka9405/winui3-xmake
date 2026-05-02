#pragma once

#include "FeedbackPage.g.h"

namespace winrt::gallery::implementation {
struct FeedbackPage : FeedbackPageT<FeedbackPage> {
    FeedbackPage();

    //  ProgressBar handlers 
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

    //  ProgressRing handlers 
    void ProgressRingActiveToggle_Toggled(
        Windows::Foundation::IInspectable const&,
        Microsoft::UI::Xaml::RoutedEventArgs const&);

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
};
} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct FeedbackPage : FeedbackPageT<FeedbackPage, implementation::FeedbackPage> {
};
} // namespace winrt::gallery::factory_implementation
