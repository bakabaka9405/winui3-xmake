#pragma once

#include "MediaPage.g.h"

namespace winrt::gallery::implementation {
struct MediaPage : MediaPageT<MediaPage> {
    MediaPage();

    void ImageWidthSlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void ImageHeightSlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void ImageStretchCombo_SelectionChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

    void DisplayNameBox_TextChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

    void PersonWidthSlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void PersonHeightSlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void IsGroupSwitch_Toggled(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::RoutedEventArgs const& args);

    void ImageOpacitySlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void ImageCornerRadiusSlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void BadgeNumberSlider_ValueChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

    void BadgeGlyphBox_TextChanged(
        Windows::Foundation::IInspectable const& sender,
        Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

private:
    bool m_isInitializing{ true };
};

} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct MediaPage : MediaPageT<MediaPage, implementation::MediaPage> {
};
} // namespace winrt::gallery::factory_implementation
