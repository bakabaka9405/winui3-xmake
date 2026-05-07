#include "MainWindow.xaml.h"
#include "pch.h"

#include <algorithm>
#include <cmath>
#include <winrt/Microsoft.UI.Input.h>

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::paint::implementation {

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Input;
using namespace winrt::Microsoft::UI::Xaml::Controls;
namespace muxm = winrt::Microsoft::UI::Xaml::Media;

// ═══════════════════════════════════════════════════════════
// 构造与初始化
// ═══════════════════════════════════════════════════════════

MainWindow::MainWindow() {
	InitializeComponent();

	// 窗口尺寸
	this->AppWindow().Resize({ 1024, 680 });

	// 监听粗细滑块值变化以更新标签
	ThicknessSlider().ValueChanged([this](
		winrt::Windows::Foundation::IInspectable const&,
		winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args)
	{
		auto value = static_cast<int>(args.NewValue());
		ThicknessLabel().Text(std::to_wstring(value) + L" px");
	});
}

// ═══════════════════════════════════════════════════════════
// CanvasControl — 绘制事件
// ═══════════════════════════════════════════════════════════

void MainWindow::PaintCanvas_Draw(
	mgcux::CanvasControl const&,
	mgcux::CanvasDrawEventArgs const& args)
{
	auto ds = args.DrawingSession();

	// 绘制所有已完成的笔触
	for (auto const& stroke : m_strokes) {
		DrawStroke(ds, stroke);
	}

	// 绘制当前进行中的笔触（实时预览）
	if (m_isDrawing && m_currentStroke.points.size() >= 1) {
		DrawStroke(ds, m_currentStroke);
	}
}

// ═══════════════════════════════════════════════════════════
// CanvasControl — 指针事件处理
// ═══════════════════════════════════════════════════════════

void MainWindow::PaintCanvas_PointerPressed(
	winrt::Windows::Foundation::IInspectable const& sender,
	PointerRoutedEventArgs const& e)
{
	// 捕获指针以确保后续事件不会丢失
	auto canvas = sender.as<mgcux::CanvasControl>();
	canvas.CapturePointer(e.Pointer());
	e.Handled(true);

	// 初始化当前笔触
	m_currentStroke.points.clear();
	m_currentStroke.points.push_back(GetCanvasPoint(e));
	m_currentStroke.tool = GetActiveTool();
	m_currentStroke.color = BrushColorPicker().Color();
	m_currentStroke.thickness = static_cast<float>(ThicknessSlider().Value());
	m_isDrawing = true;
}

void MainWindow::PaintCanvas_PointerMoved(
	winrt::Windows::Foundation::IInspectable const&,
	PointerRoutedEventArgs const& e)
{
	if (!m_isDrawing) return;

	m_currentStroke.points.push_back(GetCanvasPoint(e));
	PaintCanvas().Invalidate();
}

void MainWindow::PaintCanvas_PointerReleased(
	winrt::Windows::Foundation::IInspectable const& sender,
	PointerRoutedEventArgs const& e)
{
	if (!m_isDrawing) return;

	auto canvas = sender.as<mgcux::CanvasControl>();
	canvas.ReleasePointerCapture(e.Pointer());

	// 记录终点
	m_currentStroke.points.push_back(GetCanvasPoint(e));

	// 将当前笔触提交到历史列表
	m_strokes.push_back(std::move(m_currentStroke));
	m_currentStroke = PaintStroke{};
	m_isDrawing = false;

	PaintCanvas().Invalidate();
	UpdateButtonStates();
}

// ═══════════════════════════════════════════════════════════
// 控件事件处理
// ═══════════════════════════════════════════════════════════

void MainWindow::UndoButton_Click(
	winrt::Windows::Foundation::IInspectable const&,
	RoutedEventArgs const&)
{
	if (!m_strokes.empty()) {
		m_strokes.pop_back();
		PaintCanvas().Invalidate();
		UpdateButtonStates();
	}
}

void MainWindow::ClearButton_Click(
	winrt::Windows::Foundation::IInspectable const&,
	RoutedEventArgs const&)
{
	m_strokes.clear();
	PaintCanvas().Invalidate();
	UpdateButtonStates();
}

void MainWindow::BrushColorPicker_ColorChanged(
	ColorPicker const&,
	ColorChangedEventArgs const&)
{
	// 颜色在 PointerPressed 时从 BrushColorPicker().Color() 读取
	// 此处理函数仅用于 XAML 事件绑定，无需额外操作
}

// ═══════════════════════════════════════════════════════════
// 辅助函数
// ═══════════════════════════════════════════════════════════

DrawingTool MainWindow::GetActiveTool() {
	if (LineTool().IsChecked().GetBoolean())
		return DrawingTool::Line;
	if (RectTool().IsChecked().GetBoolean())
		return DrawingTool::Rectangle;
	if (EllipseTool().IsChecked().GetBoolean())
		return DrawingTool::Ellipse;
	return DrawingTool::Pen; // PenTool 或默认
}

void MainWindow::DrawStroke(
	mgc::CanvasDrawingSession const& ds,
	PaintStroke const& stroke)
{
	if (stroke.points.empty()) return;

	auto const color = stroke.color;
	auto const thickness = stroke.thickness;

	switch (stroke.tool) {

	// ── 画笔模式：逐段连线（自由绘制） ──────────────────────
	case DrawingTool::Pen: {
		for (size_t i = 0; i + 1 < stroke.points.size(); ++i) {
			auto const& p0 = stroke.points[i];
			auto const& p1 = stroke.points[i + 1];
			ds.DrawLine(p0.X, p0.Y, p1.X, p1.Y, color, thickness);
		}
		// 单点情况下绘制一个点
		if (stroke.points.size() == 1) {
			auto const& p = stroke.points[0];
			ds.DrawLine(p.X, p.Y, p.X + 0.5f, p.Y + 0.5f, color, thickness);
		}
		break;
	}

	// ── 直线模式 ────────────────────────────────────────────
	case DrawingTool::Line: {
		auto const& p0 = stroke.points.front();
		auto const& p1 = stroke.points.back();
		ds.DrawLine(p0.X, p0.Y, p1.X, p1.Y, color, thickness);
		break;
	}

	// ── 矩形模式 ────────────────────────────────────────────
	case DrawingTool::Rectangle: {
		auto const& p0 = stroke.points.front();
		auto const& p1 = stroke.points.back();
		auto const x = (std::min)(p0.X, p1.X);
		auto const y = (std::min)(p0.Y, p1.Y);
		auto const w = std::abs(p1.X - p0.X);
		auto const h = std::abs(p1.Y - p0.Y);
		ds.DrawRectangle(x, y, w, h, color, thickness);
		break;
	}

	// ── 椭圆模式 ────────────────────────────────────────────
	case DrawingTool::Ellipse: {
		auto const& p0 = stroke.points.front();
		auto const& p1 = stroke.points.back();
		auto const cx = (p0.X + p1.X) * 0.5f;
		auto const cy = (p0.Y + p1.Y) * 0.5f;
		auto const rx = std::abs(p1.X - p0.X) * 0.5f;
		auto const ry = std::abs(p1.Y - p0.Y) * 0.5f;
		ds.DrawEllipse(cx, cy, rx, ry, color, thickness);
		break;
	}
	}
}

void MainWindow::UpdateButtonStates() {
	auto const hasStrokes = !m_strokes.empty();
	UndoButton().IsEnabled(hasStrokes);
	ClearButton().IsEnabled(hasStrokes);
}

Point MainWindow::GetCanvasPoint(PointerRoutedEventArgs const& e) {
	auto ptrPoint = e.GetCurrentPoint(PaintCanvas());
	return ptrPoint.Position();
}

} // namespace winrt::paint::implementation
