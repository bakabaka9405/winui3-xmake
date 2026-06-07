#pragma once

#include "BoardView.g.h"

#include <vector>

namespace winrt::kanban::implementation {
struct BoardView : BoardViewT<BoardView> {
	BoardView();

	winrt::kanban::BoardViewModel ViewModel() const { return m_viewModel; }
	void ViewModel(winrt::kanban::BoardViewModel const& value) { m_viewModel = value; }
	void FinishEditIfPointerOutside(
		Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e,
		Microsoft::UI::Xaml::UIElement const& relativeTo);
	void BeginColumnHeaderDrag(winrt::hstring const& columnId);
	void CancelPendingColumnDrag();
	void BeginCardDrag();
	void EndCardDrag();
	void OnColumnDragItemsStarting(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::DragItemsStartingEventArgs const& e);
	void OnColumnDragItemsCompleted(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Controls::DragItemsCompletedEventArgs const& e);

private:
	void OnBoardPointerWheelChanged(
		winrt::Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
	std::vector<winrt::hstring> SnapshotColumnOrder() const;
	bool HasActiveColumnEdit() const;
	void ResetColumnDragState();

	winrt::kanban::BoardViewModel m_viewModel{ nullptr };
	std::vector<winrt::hstring> m_columnOrderBeforeDrag;
	winrt::hstring m_pendingColumnDragId;
	bool m_columnCanDragItemsBeforeCardDrag{ true };
	bool m_columnCanReorderItemsBeforeCardDrag{ true };
	bool m_columnDragPending{ false };
	bool m_columnDragStarted{ false };
	bool m_cardDragSuspendsColumnDrag{ false };
};
} // namespace winrt::kanban::implementation

namespace winrt::kanban::factory_implementation {
struct BoardView : BoardViewT<BoardView, implementation::BoardView> {
};
} // namespace winrt::kanban::factory_implementation
