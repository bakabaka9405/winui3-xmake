#include "pch.h"

#include "PaneSplitter.xaml.h"

#include <winrt/Microsoft.UI.Input.h>

#if __has_include("PaneSplitter.g.cpp")
#include "PaneSplitter.g.cpp"
#endif

namespace winrt::xamlstudio::implementation {
using namespace Microsoft::UI::Input;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Input;

namespace {
constexpr double SplitterInactiveWidth = 1.0;
constexpr double SplitterActiveWidth = 3.0;

double ClampPaneWidth(double value, double minimum, double maximum) {
	if (maximum < minimum) {
		return minimum;
	}
	if (value < minimum) {
		return minimum;
	}
	if (value > maximum) {
		return maximum;
	}
	return value;
}

muxm::SolidColorBrush GetThemeBrush(winrt::hstring const& resourceKey) {
	auto resources = Application::Current().Resources();
	auto value = resources.Lookup(box_value(resourceKey));
	return value.as<muxm::SolidColorBrush>();
}
} // namespace

PaneSplitter::PaneSplitter() {
	InitializeComponent();

	ProtectedCursor(InputSystemCursor::Create(InputSystemCursorShape::SizeWestEast));
	SetActiveVisual(false);

	PointerEntered({ this, &PaneSplitter::HandlePointerEntered });
	PointerExited({ this, &PaneSplitter::HandlePointerExited });
	PointerPressed({ this, &PaneSplitter::HandlePointerPressed });
	PointerMoved({ this, &PaneSplitter::HandlePointerMoved });
	PointerReleased({ this, &PaneSplitter::HandlePointerReleased });
	PointerCanceled({ this, &PaneSplitter::HandlePointerReleased });
	PointerCaptureLost({ this, &PaneSplitter::HandlePointerCaptureLost });
}

void PaneSplitter::SetActiveVisual(bool isActive) {
	SplitterLine().Width(isActive ? SplitterActiveWidth : SplitterInactiveWidth);
	SplitterLine().Background(GetThemeBrush(
		isActive ? L"AccentFillColorDefaultBrush" : L"ControlStrokeColorDefaultBrush"));
}

void PaneSplitter::HandlePointerEntered(
	winrt::Windows::Foundation::IInspectable const&,
	PointerRoutedEventArgs const& e) {
	SetActiveVisual(true);
	e.Handled(true);
}

void PaneSplitter::HandlePointerExited(
	winrt::Windows::Foundation::IInspectable const&,
	PointerRoutedEventArgs const& e) {
	if (!m_isDragging) {
		SetActiveVisual(false);
	}
	e.Handled(true);
}

void PaneSplitter::HandlePointerPressed(
	winrt::Windows::Foundation::IInspectable const&,
	PointerRoutedEventArgs const& e) {
	auto layoutRoot = Parent().try_as<Grid>();
	if (!layoutRoot || !TryBeginResize(layoutRoot)) {
		return;
	}

	CapturePointer(e.Pointer());
	m_dragStartX = e.GetCurrentPoint(layoutRoot).Position().X;
	m_isDragging = true;
	SetActiveVisual(true);
	e.Handled(true);
}

void PaneSplitter::HandlePointerMoved(
	winrt::Windows::Foundation::IInspectable const&,
	PointerRoutedEventArgs const& e) {
	if (!m_isDragging) {
		return;
	}

	auto layoutRoot = Parent().try_as<Grid>();
	if (!layoutRoot) {
		ResetResizeState();
		return;
	}

	// Convert horizontal movement into adjacent column widths.
	auto columns = layoutRoot.ColumnDefinitions();
	auto previousColumn = columns.GetAt(m_previousColumnIndex);
	auto nextColumn = columns.GetAt(m_nextColumnIndex);
	auto const position = e.GetCurrentPoint(layoutRoot).Position();
	auto const delta = position.X - m_dragStartX;
	auto const minimumPreviousWidth = previousColumn.MinWidth();
	auto const minimumNextWidth = nextColumn.MinWidth();
	auto const maximumPreviousWidth = m_adjacentPaneWidth - minimumNextWidth;
	auto const nextPreviousWidth = ClampPaneWidth(
		m_previousColumnStartWidth + delta,
		minimumPreviousWidth,
		maximumPreviousWidth);
	auto const nextNextWidth = m_adjacentPaneWidth - nextPreviousWidth;

	previousColumn.Width(GridLengthHelper::FromPixels(nextPreviousWidth));
	nextColumn.Width(GridLengthHelper::FromPixels(nextNextWidth));
	e.Handled(true);
}

void PaneSplitter::HandlePointerReleased(
	winrt::Windows::Foundation::IInspectable const&,
	PointerRoutedEventArgs const& e) {
	if (!m_isDragging) {
		return;
	}

	ReleasePointerCapture(e.Pointer());
	ResetResizeState();
	e.Handled(true);
}

void PaneSplitter::HandlePointerCaptureLost(
	winrt::Windows::Foundation::IInspectable const&,
	PointerRoutedEventArgs const& e) {
	if (m_isDragging) {
		ResetResizeState();
		e.Handled(true);
	}
}

bool PaneSplitter::TryBeginResize(Grid const& layoutRoot) {
	auto columns = layoutRoot.ColumnDefinitions();
	auto const splitterColumnIndex = static_cast<uint32_t>(Grid::GetColumn(*this));
	if (splitterColumnIndex == 0 || splitterColumnIndex + 1 >= columns.Size()) {
		return false;
	}

	m_previousColumnIndex = splitterColumnIndex - 1;
	m_nextColumnIndex = splitterColumnIndex + 1;
	auto previousColumn = columns.GetAt(m_previousColumnIndex);
	auto nextColumn = columns.GetAt(m_nextColumnIndex);
	m_previousColumnStartWidth = previousColumn.ActualWidth();
	m_adjacentPaneWidth = previousColumn.ActualWidth() + nextColumn.ActualWidth();
	return true;
}

void PaneSplitter::ResetResizeState() {
	m_isDragging = false;
	SetActiveVisual(false);
}

} // namespace winrt::xamlstudio::implementation
