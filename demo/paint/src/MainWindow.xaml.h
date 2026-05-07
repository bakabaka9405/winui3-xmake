#pragma once

#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>

#include "MainWindow.g.h"

namespace mgc = winrt::Microsoft::Graphics::Canvas;
namespace mgcux = winrt::Microsoft::Graphics::Canvas::UI::Xaml;

namespace winrt::paint::implementation {

// 绘图工具枚举
enum class DrawingTool {
	Pen,
	Line,
	Rectangle,
	Ellipse,
};

// 单次笔触的数据结构
struct PaintStroke {
	std::vector<winrt::Windows::Foundation::Point> points;
	DrawingTool tool;
	winrt::Windows::UI::Color color;
	float thickness;
};

struct MainWindow : MainWindowT<MainWindow> {
	MainWindow();

	// ── CanvasControl 事件 ──────────────────────────────────
	void PaintCanvas_Draw(
		mgcux::CanvasControl const& sender,
		mgcux::CanvasDrawEventArgs const& args);

	void PaintCanvas_PointerPressed(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

	void PaintCanvas_PointerMoved(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

	void PaintCanvas_PointerReleased(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

	// ── 控件事件 ────────────────────────────────────────────
	void UndoButton_Click(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void ClearButton_Click(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void BrushColorPicker_ColorChanged(
		winrt::Microsoft::UI::Xaml::Controls::ColorPicker const& sender,
		winrt::Microsoft::UI::Xaml::Controls::ColorChangedEventArgs const& args);

private:
	// ── 辅助函数 ────────────────────────────────────────────
	DrawingTool GetActiveTool();
	void DrawStroke(mgc::CanvasDrawingSession const& ds, PaintStroke const& stroke);
	void UpdateButtonStates();
	winrt::Windows::Foundation::Point GetCanvasPoint(
		winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

	// ── 绘图状态 ────────────────────────────────────────────
	std::vector<PaintStroke> m_strokes;
	PaintStroke m_currentStroke{};
	bool m_isDrawing = false;
};

} // namespace winrt::paint::implementation

namespace winrt::paint::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
};
} // namespace winrt::paint::factory_implementation
