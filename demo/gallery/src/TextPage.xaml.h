#pragma once

#include "TextPage.g.h"

namespace winrt::gallery::implementation {
struct TextPage : TextPageT<TextPage> {
	TextPage();

private:
	bool m_updatingText{ false };
	bool m_updatingNumber{ false };
	bool m_isInitializing{ true };

public:
	//  Reverse binding handlers
	void DemoTextBox_TextChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

	void DemoNumberBox_ValueChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs const& args);

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

	void OnTextBoxPlaceholderChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

	void OnTextBoxMaxLengthChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void OnTextBoxFontSizeChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void OnTextBoxTextAlignmentChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

	//  PasswordBox property editor handlers
	void OnPasswordRevealModeChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

	void OnPasswordRevealButtonToggled(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);

	void OnPasswordBoxWidthChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void OnPasswordBoxPlaceholderChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

	//  NumberBox property editor handlers
	void OnNumberBoxValueChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void OnSpinButtonModeChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

	void OnNumberBoxMinChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void OnNumberBoxMaxChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void OnNumberBoxWidthChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	//  RichEditBox property editor handlers
	void OnRichEditReadOnlyToggled(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);

	void OnRichEditSpellCheckToggled(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);

	void OnRichEditHeightChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	void OnRichEditWidthChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

	//  AutoSuggestBox property editor handlers
	void OnAutoSuggestUpdateToggled(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);

	void OnAutoSuggestPlaceholderChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);

	void OnAutoSuggestWidthChanged(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);
};
} // namespace winrt::gallery::implementation

namespace winrt::gallery::factory_implementation {
struct TextPage : TextPageT<TextPage, implementation::TextPage> {
};
} // namespace winrt::gallery::factory_implementation
