#include "MediaPage.xaml.h"
#include "pch.h"

#if __has_include("MediaPage.g.cpp")
#include "MediaPage.g.cpp"
#endif

namespace winrt::gallery::implementation {

MediaPage::MediaPage() {
    InitializeComponent();
}

// ============================================================================
// Image property editors
// ============================================================================
void MediaPage::ImageWidthSlider_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    double value = ImageWidthSlider().Value();

    DemoImage().Width(value);

    ImageWidthLabel().Text(
        hstring{L"Width: " + std::to_wstring(static_cast<int32_t>(value))});
}

void MediaPage::ImageHeightSlider_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    double value = ImageHeightSlider().Value();

    DemoImage().Height(value);

    ImageHeightLabel().Text(
        hstring{L"Height: " + std::to_wstring(static_cast<int32_t>(value))});
}

void MediaPage::ImageStretchCombo_SelectionChanged(
    wf::IInspectable const&,
    muxc::SelectionChangedEventArgs const&) {
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

// ============================================================================
// PersonPicture property editors
// ============================================================================
void MediaPage::DisplayNameBox_TextChanged(
    wf::IInspectable const&,
    muxc::TextChangedEventArgs const&) {
    hstring name = DisplayNameBox().Text();

    DemoPersonPicture().DisplayName(name);
    DemoPersonPicProfile().DisplayName(name);
}

void MediaPage::PersonWidthSlider_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    double value = PersonWidthSlider().Value();

    DemoPersonPicture().Width(value);
    DemoPersonPicProfile().Width(value);

    PersonWidthLabel().Text(
        hstring{L"Width: " + std::to_wstring(static_cast<int32_t>(value))});
}

void MediaPage::PersonHeightSlider_ValueChanged(
    wf::IInspectable const&,
    muxp::RangeBaseValueChangedEventArgs const&) {
    double value = PersonHeightSlider().Value();

    DemoPersonPicture().Height(value);
    DemoPersonPicProfile().Height(value);

    PersonHeightLabel().Text(
        hstring{L"Height: " + std::to_wstring(static_cast<int32_t>(value))});
}

void MediaPage::IsGroupSwitch_Toggled(
    wf::IInspectable const&,
    mux::RoutedEventArgs const&) {
    bool isGroup = IsGroupSwitch().IsOn();

    DemoPersonPicture().IsGroup(isGroup);
    DemoPersonPicProfile().IsGroup(isGroup);
}

} // namespace winrt::gallery::implementation
