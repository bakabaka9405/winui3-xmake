#include "SelectionPage.xaml.h"
#include "pch.h"

#if __has_include("SelectionPage.g.cpp")
#include "SelectionPage.g.cpp"
#endif

namespace winrt::gallery::implementation {

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
	DemoListBox().Width(ListBoxWidthSlider().Value());
}

void SelectionPage::OnListBoxHeightChanged(
	wf::IInspectable const&,
	muxp::RangeBaseValueChangedEventArgs const&) {
	DemoListBox().Height(ListBoxHeightSlider().Value());
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
	DemoListView().SelectionMode(static_cast<muxc::ListViewSelectionMode>(idx + 1));
}

void SelectionPage::OnListViewItemClickToggled(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	DemoListView().IsItemClickEnabled(ListViewItemClickToggle().IsOn());
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
	DemoGridView().SelectionMode(static_cast<muxc::ListViewSelectionMode>(idx + 1));
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
