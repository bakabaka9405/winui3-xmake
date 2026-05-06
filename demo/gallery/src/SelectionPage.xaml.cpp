#include "SelectionPage.xaml.h"
#include "pch.h"

#if __has_include("SelectionPage.g.cpp")
#include "SelectionPage.g.cpp"
#endif

namespace winrt::gallery::implementation {

// ============================================================================
// Constructor & data population
// ============================================================================

SelectionPage::SelectionPage() {
	InitializeComponent();

	// --- Populate shared color data ---
	auto colors = winrt::single_threaded_observable_vector<hstring>();
	for (auto&& c : { L"Red", L"Green", L"Blue", L"Yellow",
					  L"Purple", L"Orange", L"Cyan", L"Magenta" }) {
		colors.Append(c);
	}

	DemoEditableComboBox().ItemsSource(colors);
	DemoListBox().ItemsSource(colors);
	DemoListView().ItemsSource(colors);
	DemoGridView().ItemsSource(colors);
}

// ============================================================================
// Helper: format a slider label string
// ============================================================================

namespace {
hstring FormatSliderLabel(std::wstring_view prefix, double value) {
	return hstring{ std::wstring{ prefix } + L": " + std::to_wstring(static_cast<int>(value)) };
}
} // namespace

// ============================================================================
// ComboBox handlers
// ============================================================================

void SelectionPage::OnComboSelectionChanged(
	wf::IInspectable const& sender,
	muxc::SelectionChangedEventArgs const&) {
	auto combo = sender.as<muxc::ComboBox>();
	auto selected = combo.SelectedItem();

	if (!selected) {
		StatusText().Text(L"Selection cleared");
		return;
	}

	hstring text;

	// Static ComboBox: selected item is a ComboBoxItem → extract Content
	auto item = selected.try_as<muxc::ComboBoxItem>();
	if (item) {
		text = winrt::unbox_value_or<hstring>(item.Content(), L"");
	}
	else {
		// Data-bound ComboBox: selected item is the data value directly
		text = winrt::unbox_value_or<hstring>(selected, L"");
	}

	StatusText().Text(hstring{ L"ComboBox selected: " } + text);
}

void SelectionPage::OnComboEditableToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoEditableComboBox().IsEditable(ComboEditableToggle().IsOn());
}

void SelectionPage::OnComboWidthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	double w = ComboWidthSlider().Value();
	DemoComboBox().Width(w);
	ComboWidthLabel().Text(FormatSliderLabel(L"Width", w));
}

void SelectionPage::OnComboPlaceholderChanged(
	wf::IInspectable const&,
	muxc::TextChangedEventArgs const&) {
	DemoEditableComboBox().PlaceholderText(ComboPlaceholderBox().Text());
}

void SelectionPage::OnComboDropDownHeightChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	double h = ComboDropDownSlider().Value();
	DemoEditableComboBox().MaxDropDownHeight(h);
	ComboDropDownLabel().Text(FormatSliderLabel(L"MaxDropDownHeight", h));
}

// ============================================================================
// ListBox handlers
// ============================================================================

void SelectionPage::OnListBoxSelectionChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	auto formatted = FormatSelectedItems(DemoListBox().SelectedItems());
	StatusText().Text(L"ListBox selected: " + formatted);
}

void SelectionPage::OnListBoxSelectionModeChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	auto mode = muxc::SelectionMode::Single;
	switch (ListBoxSelectionMode().SelectedIndex()) {
	case 0: mode = muxc::SelectionMode::Single; break;
	case 1: mode = muxc::SelectionMode::Multiple; break;
	case 2: mode = muxc::SelectionMode::Extended; break;
	}
	DemoListBox().SelectionMode(mode);
}

void SelectionPage::OnListBoxWidthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	double w = ListBoxWidthSlider().Value();
	DemoListBox().Width(w);
	ListBoxWidthLabel().Text(FormatSliderLabel(L"Width", w));
}

void SelectionPage::OnListBoxHeightChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	double h = ListBoxHeightSlider().Value();
	DemoListBox().Height(h);
	ListBoxHeightLabel().Text(FormatSliderLabel(L"Height", h));
}

void SelectionPage::OnListBoxEnabledToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoListBox().IsEnabled(ListBoxEnabledToggle().IsOn());
}

// ============================================================================
// ListView handlers
// ============================================================================

void SelectionPage::OnListViewSelectionChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	auto formatted = FormatSelectedItems(DemoListView().SelectedItems());
	StatusText().Text(L"ListView selected: " + formatted);
}

void SelectionPage::OnListViewSelectionModeChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	int32_t idx = ListViewSelectionMode().SelectedIndex();
	DemoListView().SelectionMode(static_cast<muxc::ListViewSelectionMode>(idx));
}

void SelectionPage::OnListViewItemClickToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoListView().IsItemClickEnabled(ListViewItemClickToggle().IsOn());
}

void SelectionPage::OnListViewWidthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	double w = ListViewWidthSlider().Value();
	DemoListView().Width(w);
	ListViewWidthLabel().Text(FormatSliderLabel(L"Width", w));
}

void SelectionPage::OnListViewHeightChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	double h = ListViewHeightSlider().Value();
	DemoListView().Height(h);
	ListViewHeightLabel().Text(FormatSliderLabel(L"Height", h));
}

void SelectionPage::OnListViewEnabledToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoListView().IsEnabled(ListViewEnabledToggle().IsOn());
}

void SelectionPage::OnListViewScrollingPlaceholdersToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoListView().ShowsScrollingPlaceholders(ListViewScrollingToggle().IsOn());
}

// ============================================================================
// GridView handlers
// ============================================================================

void SelectionPage::OnGridViewSelectionChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	auto formatted = FormatSelectedItems(DemoGridView().SelectedItems());
	StatusText().Text(L"GridView selected: " + formatted);
	UpdateGridViewInfo();
}

void SelectionPage::OnGridViewSelectionModeChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	int32_t idx = GridViewSelectionMode().SelectedIndex();
	DemoGridView().SelectionMode(static_cast<muxc::ListViewSelectionMode>(idx));
}

void SelectionPage::OnGridViewWidthChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	double w = GridViewWidthSlider().Value();
	DemoGridView().Width(w);
	GridViewWidthLabel().Text(FormatSliderLabel(L"Width", w));
}

void SelectionPage::OnGridViewHeightChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	double h = GridViewHeightSlider().Value();
	DemoGridView().Height(h);
	GridViewHeightLabel().Text(FormatSliderLabel(L"Height", h));
}

void SelectionPage::OnGridViewEnabledToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoGridView().IsEnabled(GridViewEnabledToggle().IsOn());
}

// ============================================================================
// Helpers
// ============================================================================

hstring SelectionPage::FormatSelectedItems(
	wfc::IVector<wf::IInspectable> const& items) {
	if (items.Size() == 0) {
		return L"(none)";
	}

	std::wstring result;
	for (uint32_t i = 0; i < items.Size(); ++i) {
		if (i > 0) {
			result += L", ";
		}
		result += winrt::unbox_value<hstring>(items.GetAt(i));
	}
	return hstring{ result };
}

void SelectionPage::UpdateGridViewInfo() {
	auto count = DemoGridView().SelectedItems().Size();
	GridViewItemInfo().Text(
		hstring{ L"Selected items: " + std::to_wstring(count) });
}

} // namespace winrt::gallery::implementation
