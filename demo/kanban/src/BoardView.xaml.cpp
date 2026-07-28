#include "pch.h"

#include "BoardView.xaml.h"

#include "BoardColumn.xaml.h"
#include "BoardViewModel.h"

#include <algorithm>
#include <winrt/Microsoft.UI.Input.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>

#if __has_include("BoardView.g.cpp")
#include "BoardView.g.cpp"
#endif

namespace winrt::kanban::implementation {
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Input;
using namespace winrt::Microsoft::UI::Xaml::Media;
namespace wadt = winrt::Windows::ApplicationModel::DataTransfer;
namespace ws = winrt::Windows::System;

static bool FinishColumnEditIfPointerOutside(
	DependencyObject const& root,
	PointerRoutedEventArgs const& e,
	UIElement const& relativeTo) {
	if (!root) return false;

	if (auto column = root.try_as<winrt::kanban::BoardColumn>()) {
		auto impl = winrt::get_self<implementation::BoardColumn>(column);
		if (impl->IsEditing()) {
			impl->FinishEditIfPointerOutside(e, relativeTo);
			return true;
		}
	}

	auto const childCount = VisualTreeHelper::GetChildrenCount(root);
	for (int32_t i = 0; i < childCount; i++) {
		if (FinishColumnEditIfPointerOutside(VisualTreeHelper::GetChild(root, i), e, relativeTo)) {
			return true;
		}
	}
	return false;
}

static bool HasEditingColumn(DependencyObject const& root) {
	if (!root) return false;

	if (auto column = root.try_as<winrt::kanban::BoardColumn>()) {
		auto impl = winrt::get_self<implementation::BoardColumn>(column);
		if (impl->IsEditing()) {
			return true;
		}
	}

	auto const childCount = VisualTreeHelper::GetChildrenCount(root);
	for (int32_t i = 0; i < childCount; i++) {
		if (HasEditingColumn(VisualTreeHelper::GetChild(root, i))) {
			return true;
		}
	}
	return false;
}

BoardView::BoardView() {
	m_viewModel = winrt::make<BoardViewModel>();
	InitializeComponent();
	BoardWheelSurface().AddHandler(
		UIElement::PointerWheelChangedEvent(),
		winrt::box_value(PointerEventHandler{ this, &BoardView::OnBoardPointerWheelChanged }),
		true);

	// XAML 树构造完成后加载已保存数据，避免阻塞 UI 初始化
	Loaded([this](auto&&, auto&&) {
		winrt::get_self<implementation::BoardViewModel>(m_viewModel)->Load();
	});
	Unloaded([this](auto&&, auto&&) {
		EndCardDrag();
	});
}

void BoardView::FinishEditIfPointerOutside(
	PointerRoutedEventArgs const& e,
	UIElement const& relativeTo) {
	FinishColumnEditIfPointerOutside(*this, e, relativeTo);
}

void BoardView::BeginColumnHeaderDrag(winrt::hstring const& columnId) {
	if (columnId.empty() || HasActiveColumnEdit()) return;

	m_pendingColumnDragId = columnId;
	m_columnDragPending = true;
	m_columnDragStarted = false;
}

void BoardView::CancelPendingColumnDrag() {
	if (m_columnDragStarted) return;
	ResetColumnDragState();
}

void BoardView::BeginCardDrag() {
	if (m_cardDragSuspendsColumnDrag) return;

	ResetColumnDragState();
	m_columnCanDragItemsBeforeCardDrag = ColumnRepeater().CanDragItems();
	m_columnCanReorderItemsBeforeCardDrag = ColumnRepeater().CanReorderItems();
	ColumnRepeater().CanDragItems(false);
	ColumnRepeater().CanReorderItems(false);
	m_cardDragSuspendsColumnDrag = true;
}

void BoardView::EndCardDrag() {
	if (!m_cardDragSuspendsColumnDrag) return;

	ColumnRepeater().CanDragItems(m_columnCanDragItemsBeforeCardDrag);
	ColumnRepeater().CanReorderItems(m_columnCanReorderItemsBeforeCardDrag);
	m_cardDragSuspendsColumnDrag = false;
}

void BoardView::OnColumnDragItemsStarting(
	IInspectable const&,
	DragItemsStartingEventArgs const& e) {
	if (!m_columnDragPending || HasActiveColumnEdit() || e.Items().Size() != 1) {
		e.Cancel(true);
		ResetColumnDragState();
		return;
	}

	auto column = e.Items().GetAt(0).try_as<winrt::kanban::ColumnData>();
	if (!column || column.Id() != m_pendingColumnDragId) {
		e.Cancel(true);
		ResetColumnDragState();
		return;
	}

	m_columnOrderBeforeDrag = SnapshotColumnOrder();
	m_columnDragStarted = true;
	e.Data().RequestedOperation(wadt::DataPackageOperation::Move);
}

void BoardView::OnColumnDragItemsCompleted(
	IInspectable const&,
	DragItemsCompletedEventArgs const&) {
	if (m_columnDragStarted && m_viewModel && m_columnOrderBeforeDrag != SnapshotColumnOrder()) {
		m_viewModel.Save();
	}

	ResetColumnDragState();
}

void BoardView::OnBoardPointerWheelChanged(
	IInspectable const&,
	PointerRoutedEventArgs const& e) {
	if ((e.KeyModifiers() & ws::VirtualKeyModifiers::Shift) != ws::VirtualKeyModifiers::Shift) return;
	e.Handled(true);

	auto const delta = e.GetCurrentPoint(ColumnRepeater()).Properties().MouseWheelDelta();
	if (delta == 0) return;

	auto scrollViewer = BoardScrollViewer();
	auto const nextOffset = std::clamp(
		scrollViewer.HorizontalOffset() - static_cast<double>(delta),
		0.0,
		scrollViewer.ScrollableWidth());

	scrollViewer.ChangeView(
		winrt::Windows::Foundation::IReference<double>{ nextOffset },
		nullptr,
		nullptr,
		true);
}

std::vector<winrt::hstring> BoardView::SnapshotColumnOrder() const {
	std::vector<winrt::hstring> order;
	if (!m_viewModel) return order;

	auto columns = m_viewModel.Columns();
	order.reserve(columns.Size());
	for (auto const& column : columns) {
		order.push_back(column.Id());
	}
	return order;
}

bool BoardView::HasActiveColumnEdit() const {
	return HasEditingColumn(*this);
}

void BoardView::ResetColumnDragState() {
	m_columnOrderBeforeDrag.clear();
	m_pendingColumnDragId = L"";
	m_columnDragPending = false;
	m_columnDragStarted = false;
}

} // namespace winrt::kanban::implementation
