#pragma once

#include "TextPage.g.h"

namespace winrt::gallery::implementation {
struct TextPage : TextPageT<TextPage> {
	TextPage();

	//  TextBox property editor handlers 
	void OnTextBoxTextChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

	void OnTextBoxWidthChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void OnTextBoxReadOnlyToggled(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);

	void OnTextBoxHeaderChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

	//  PasswordBox property editor handlers 
	void OnPasswordRevealModeChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

	void OnPasswordRevealButtonToggled(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);

	//  NumberBox property editor handlers 
	void OnNumberBoxValueChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void OnSpinButtonModeChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

	//  RichEditBox property editor handlers 
	void OnRichEditReadOnlyToggled(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);

	void OnRichEditSpellCheckToggled(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);

	//  AutoSuggestBox property editor handler 
	void OnAutoSuggestUpdateToggled(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);
};
} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct TextPage : TextPageT<TextPage, implementation::TextPage> {
};
} // namespace winrt::gallery::factory_implementation
