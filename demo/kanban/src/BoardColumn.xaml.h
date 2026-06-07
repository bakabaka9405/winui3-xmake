#pragma once

#include "BoardColumn.g.h"

#include <chrono>

namespace winrt::kanban::implementation {
struct BoardColumn : BoardColumnT<BoardColumn> {
	BoardColumn();

	winrt::kanban::ColumnData ColumnData() const { return m_columnData; }
	void ColumnData(winrt::kanban::ColumnData const& value);

	winrt::kanban::BoardViewModel ViewModel() const { return m_viewModel; }
	void ViewModel(winrt::kanban::BoardViewModel const& value) { m_viewModel = value; }
	bool IsEditing() const;
	void FinishEditIfPointerOutside(
		Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e,
		Microsoft::UI::Xaml::UIElement const& relativeTo);

	void OnLoaded(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void OnDeleteColumn(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void OnAddCard(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void OnToggleCollapsed(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void OnDeleteCard(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void OnDragItemsStarting(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::DragItemsStartingEventArgs const& e);
	void OnDragItemsCompleted(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::DragItemsCompletedEventArgs const& e);

	void OnListDragOver(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::DragEventArgs const& e);

	void OnListDrop(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::DragEventArgs const& e);

	void OnColumnTitleDoubleTapped(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const& e);
	void OnColumnTitleKeyDown(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e);
	void OnEditorLosingFocus(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Input::LosingFocusEventArgs const& e);
	void OnColumnTitleLostFocus(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void OnCardTitleDoubleTapped(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const& e);
	void OnCardTitleKeyDown(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e);
	void OnCardTitleLostFocus(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void OnCardDescriptionDoubleTapped(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const& e);
	void OnCardDescriptionLostFocus(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& e);
	void OnCardPointerWheelChanged(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

private:
	enum class EditField {
		None,
		ColumnTitle,
		CardTitle,
		CardDescription
	};

	void UpdateHeader();
	void UpdateCollapsedState();
	void OnPointerPressedForColumnDrag(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
	void OnPointerReleasedForColumnDrag(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
	bool IsColumnHeaderBlankPoint(
		winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
	void BeginColumnHeaderDrag();
	void CancelColumnHeaderDrag();
	void BeginColumnTitleEdit();
	void FinishActiveEdit(bool commit);
	void FinishColumnTitleEdit(bool commit);
	void ResetEditState();
	void SuppressInitialLostFocus();
	void FocusActiveEditorWhenReady();
	void GuardKeyboardCommitFocus(
		winrt::Microsoft::UI::Xaml::Controls::TextBox const& editor);
	void ClearKeyboardCommitFocusGuardWhenReady();
	bool ShouldSuppressInitialLostFocus(
		winrt::Microsoft::UI::Xaml::Controls::TextBox const& editor);
	void BeginCardEdit(
		winrt::Microsoft::UI::Xaml::DependencyObject const& source,
		EditField field);
	void FinishCardEdit(
		winrt::Microsoft::UI::Xaml::Controls::TextBox const& editor,
		bool commit);
	void SetEditingDragState(bool editing);

	winrt::kanban::ColumnData m_columnData{ nullptr };
	winrt::kanban::BoardViewModel m_viewModel{ nullptr };
	EditField m_activeField{ EditField::None };
	winrt::kanban::CardData m_activeCard{ nullptr };
	winrt::Microsoft::UI::Xaml::Controls::TextBox m_activeEditor{ nullptr };
	winrt::Microsoft::UI::Xaml::Controls::TextBox m_keyboardCommitEditor{ nullptr };
	winrt::Microsoft::UI::Xaml::Controls::TextBlock m_activeText{ nullptr };
	winrt::hstring m_originalText;
	bool m_finishingEdit{ false };
	bool m_initialLostFocusRefocusQueued{ false };
	std::chrono::steady_clock::time_point m_initialLostFocusSuppressUntil{};
};
} // namespace winrt::kanban::implementation

namespace winrt::kanban::factory_implementation {
struct BoardColumn : BoardColumnT<BoardColumn, implementation::BoardColumn> {
};
} // namespace winrt::kanban::factory_implementation
