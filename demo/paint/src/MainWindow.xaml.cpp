#include "MainWindow.xaml.h"
#include "pch.h"

#include <algorithm>
#include <array>
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
using namespace winrt::Microsoft::UI::Xaml::Controls::Primitives;
using namespace winrt::Microsoft::UI::Xaml::Shapes;

muxm::SolidColorBrush GetThemeBrush(winrt::hstring const& resourceKey) {
	auto resources = Application::Current().Resources();
	auto value = resources.Lookup(box_value(resourceKey));
	auto brush = value.as<muxm::SolidColorBrush>();

	return brush;
}

// ═══════════════════════════════════════════════════════════
// 构造与初始化
// ═══════════════════════════════════════════════════════════

MainWindow::MainWindow() {
	InitializeComponent();

	// 窗口尺寸
	this->AppWindow().Resize({ 1440, 900 });

	// 动态生成预制色按钮
	BuildPresetColors();

	// 监听粗细滑块值变化以更新标签
	ThicknessSlider().ValueChanged([this](
									   winrt::Windows::Foundation::IInspectable const&,
									   RangeBaseValueChangedEventArgs const& args) {
		auto value = static_cast<int>(args.NewValue());
		ThicknessLabel().Text(std::to_wstring(value) + L" px");
	});

	// 标记初始化完成 —— 此后事件处理程序方可访问全部控件
	m_isInitialized = true;
}

// ═══════════════════════════════════════════════════════════
// CanvasControl — 绘制事件
// ═══════════════════════════════════════════════════════════

void MainWindow::PaintCanvas_Draw(
	mgcux::CanvasControl const&,
	mgcux::CanvasDrawEventArgs const& args) {
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
	PointerRoutedEventArgs const& e) {
	// 捕获指针以确保后续事件不会丢失
	auto canvas = sender.as<mgcux::CanvasControl>();
	canvas.CapturePointer(e.Pointer());
	e.Handled(true);

	// 初始化当前笔触
	m_currentStroke.points.clear();
	m_currentStroke.points.push_back(GetCanvasPoint(e));
	m_currentStroke.tool = GetActiveTool();

	// 防御性读取 —— Flyout 内的 ColorPicker 可能在名称作用域中为空
	auto picker = BrushColorPicker();
	m_currentStroke.color = picker ? picker.Color() : winrt::Windows::UI::Colors::Black();
	m_currentStroke.thickness = static_cast<float>(ThicknessSlider().Value());
	m_isDrawing = true;
}

void MainWindow::PaintCanvas_PointerMoved(
	winrt::Windows::Foundation::IInspectable const&,
	PointerRoutedEventArgs const& e) {
	if (!m_isDrawing) return;

	m_currentStroke.points.push_back(GetCanvasPoint(e));
	PaintCanvas().Invalidate();
}

void MainWindow::PaintCanvas_PointerReleased(
	winrt::Windows::Foundation::IInspectable const& sender,
	PointerRoutedEventArgs const& e) {
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
// 工具切换 — ToggleButton 事件处理
// ═══════════════════════════════════════════════════════════

void MainWindow::ToolButton_Checked(
	winrt::Windows::Foundation::IInspectable const& sender,
	RoutedEventArgs const&) {
	if (!m_isInitialized) return;
	if (m_isUpdatingTools) return;

	auto button = sender.try_as<ToggleButton>();
	if (!button) return;

	if (button == PenToolButton())
		UpdateToolButtonStates(DrawingTool::Pen);
	else if (button == LineToolButton())
		UpdateToolButtonStates(DrawingTool::Line);
	else if (button == RectToolButton())
		UpdateToolButtonStates(DrawingTool::Rectangle);
	else if (button == EllipseToolButton())
		UpdateToolButtonStates(DrawingTool::Ellipse);

	auto checkedBrush = GetThemeBrush(L"TextOnAccentFillColorPrimaryBrush");

	if (sender.as<ToggleButton>() == LineToolButton()) {
		LineButtonIcon().Stroke(checkedBrush);
	}
	else if (sender.as<ToggleButton>() == RectToolButton()) {
		RectButtonIcon().BorderBrush(checkedBrush);
	}
}

void MainWindow::ToolButton_Unchecked(
	winrt::Windows::Foundation::IInspectable const& sender,
	RoutedEventArgs const&) {
	if (!m_isInitialized) return;
	// 阻止取消选中最后一个工具 —— 强制重新选中
	if (m_isUpdatingTools) return;

	auto button = sender.try_as<ToggleButton>();
	if (!button) return;

	button.IsChecked(true);
}

void MainWindow::UpdateToolButtonStates(DrawingTool tool) {
	m_isUpdatingTools = true;

	m_activeTool = tool;
	auto normalBrush = GetThemeBrush(L"TextFillColorPrimaryBrush");

	LineButtonIcon().Stroke(normalBrush);
	RectButtonIcon().BorderBrush(normalBrush);

	PenToolButton().IsChecked(tool == DrawingTool::Pen);
	LineToolButton().IsChecked(tool == DrawingTool::Line);
	RectToolButton().IsChecked(tool == DrawingTool::Rectangle);
	EllipseToolButton().IsChecked(tool == DrawingTool::Ellipse);

	m_isUpdatingTools = false;
}

// ═══════════════════════════════════════════════════════════
// 取色器 — 颜色预览 与 预制色
// ═══════════════════════════════════════════════════════════

void MainWindow::UpdateColorPreview() {
	auto picker = BrushColorPicker();
	if (!picker) return;

	auto color = picker.Color();
	ColorPreviewBrush().Color(color);
}

void MainWindow::BrushColorPicker_ColorChanged(
	ColorPicker const&,
	ColorChangedEventArgs const&) {
	if (!m_isInitialized) return;
	// 更新颜色预览色块，使其与 ColorPicker 当前颜色同步
	UpdateColorPreview();
}

void MainWindow::BuildPresetColors() {
	// ── 预制色定义表（名称 + RGB） ─────────────────────────
	struct PresetColorDef {
		std::wstring_view tooltip;
		uint8_t r, g, b;
	};

	static constexpr std::array<PresetColorDef, 20> s_colors = { {
		{ L"黑色", 0x00, 0x00, 0x00 },
		{ L"深灰", 0x44, 0x44, 0x44 },
		{ L"灰色", 0x88, 0x88, 0x88 },
		{ L"浅灰", 0xCC, 0xCC, 0xCC },
		{ L"白色", 0xFF, 0xFF, 0xFF },
		{ L"深红", 0x8B, 0x00, 0x00 },
		{ L"红色", 0xFF, 0x00, 0x00 },
		{ L"橙色", 0xFF, 0x8C, 0x00 },
		{ L"金色", 0xFF, 0xD7, 0x00 },
		{ L"黄色", 0xFF, 0xFF, 0x00 },
		{ L"绿色", 0x00, 0x80, 0x00 },
		{ L"深绿", 0x00, 0x64, 0x00 },
		{ L"青色", 0x00, 0xFF, 0xFF },
		{ L"蓝色", 0x00, 0x00, 0xFF },
		{ L"深蓝", 0x00, 0x00, 0x8B },
		{ L"紫色", 0x80, 0x00, 0x80 },
		{ L"品红", 0xFF, 0x00, 0xFF },
		{ L"棕色", 0x8B, 0x45, 0x13 },
		{ L"粉色", 0xFF, 0x69, 0xB4 },
		{ L"橙红", 0xFF, 0x45, 0x00 },
	} };

	// ── 创建两行布局 ──────────────────────────────────────
	auto row1 = StackPanel();
	row1.Orientation(Orientation::Horizontal);
	row1.Spacing(4);

	auto row2 = StackPanel();
	row2.Orientation(Orientation::Horizontal);
	row2.Spacing(4);

	for (size_t i = 0; i < s_colors.size(); ++i) {
		auto const& def = s_colors[i];

		// 构造颜色
		winrt::Windows::UI::Color color{
			.A = 255, .R = def.r, .G = def.g, .B = def.b
		};

		// 色块 Rectangle
		auto rect = Rectangle();
		rect.Width(16);
		rect.Height(16);
		rect.RadiusX(2);
		rect.RadiusY(2);
		rect.Fill(muxm::SolidColorBrush(color));

		// 按钮
		auto btn = Button();
		btn.Width(24);
		btn.Height(24);
		btn.Padding(Thickness{ 0 });
		btn.Content(rect);
		btn.Click({ this, &MainWindow::PresetColor_Click });
		ToolTipService::SetToolTip(btn, winrt::box_value(def.tooltip));

		// 分配到两行
		if (i < 10) row1.Children().Append(btn);
		else row2.Children().Append(btn);
	}

	PresetColorPanel().Children().Append(row1);
	PresetColorPanel().Children().Append(row2);
}

void MainWindow::PresetColor_Click(
	winrt::Windows::Foundation::IInspectable const& sender,
	RoutedEventArgs const&) {
	// 安全向下转型：尝试从 Button → Rectangle → SolidColorBrush
	auto button = sender.try_as<Button>();
	if (!button) return;

	auto rect = button.Content().try_as<Rectangle>();
	if (!rect) return;

	auto brush = rect.Fill().try_as<muxm::SolidColorBrush>();
	if (!brush) return;

	auto color = brush.Color();

	// 将预制色设置为 ColorPicker 的当前颜色（防御性空值检查）
	auto picker = BrushColorPicker();
	if (picker) {
		picker.Color(color);
		UpdateColorPreview();
	}

	// 选择颜色后关闭 Flyout
	ColorButton().Flyout().Hide();
}

// ═══════════════════════════════════════════════════════════
// 操作按钮 — 撤销 / 清空
// ═══════════════════════════════════════════════════════════

void MainWindow::UndoButton_Click(
	winrt::Windows::Foundation::IInspectable const&,
	RoutedEventArgs const&) {
	if (!m_strokes.empty()) {
		m_strokes.pop_back();
		PaintCanvas().Invalidate();
		UpdateButtonStates();
	}
}

void MainWindow::ClearButton_Click(
	winrt::Windows::Foundation::IInspectable const&,
	RoutedEventArgs const&) {
	m_strokes.clear();
	PaintCanvas().Invalidate();
	UpdateButtonStates();
}

// ═══════════════════════════════════════════════════════════
// 辅助函数
// ═══════════════════════════════════════════════════════════

DrawingTool MainWindow::GetActiveTool() {
	return m_activeTool;
}

void MainWindow::DrawStroke(
	mgc::CanvasDrawingSession const& ds,
	PaintStroke const& stroke) {
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
