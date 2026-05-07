#include "pch.h"
#include "MainWindow.xaml.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <winrt/Microsoft.Graphics.Canvas.Geometry.h>
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
	m_pointerFilter.Reset();
	// 首个点亦经过滤波（首次调用返回原始值，随后逐渐平滑）
	m_currentStroke.points.push_back(m_pointerFilter.Step(
		GetCanvasPoint(e), std::chrono::steady_clock::now()));
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

	auto const raw = GetCanvasPoint(e);

	// One Euro 低通滤波：消除手抖微颤
	auto const pt = m_pointerFilter.Step(raw, std::chrono::steady_clock::now());

	// 距离过滤：距上一有效点 < 2px 则跳过，减少冗余采样
	if (!m_currentStroke.points.empty()) {
		auto const& last = m_currentStroke.points.back();
		float const dx = pt.X - last.X;
		float const dy = pt.Y - last.Y;
		if (dx * dx + dy * dy < 4.0f) {  // 2² = 4
			return;
		}
	}

	m_currentStroke.points.push_back(pt);
	PaintCanvas().Invalidate();
}

void MainWindow::PaintCanvas_PointerReleased(
	winrt::Windows::Foundation::IInspectable const& sender,
	PointerRoutedEventArgs const& e) {
	if (!m_isDrawing) return;

	auto canvas = sender.as<mgcux::CanvasControl>();
	canvas.ReleasePointerCapture(e.Pointer());

	// 记录终点（经 One Euro 滤波与距离过滤，避免末端折角）
	auto const raw = GetCanvasPoint(e);
	auto const pt = m_pointerFilter.Step(raw, std::chrono::steady_clock::now());
	if (!m_currentStroke.points.empty()) {
		auto const& last = m_currentStroke.points.back();
		float const dx = pt.X - last.X;
		float const dy = pt.Y - last.Y;
		if (dx * dx + dy * dy >= 4.0f) {  // 距上一点 ≥2px 才记录
			m_currentStroke.points.push_back(pt);
		}
	} else {
		m_currentStroke.points.push_back(pt);
	}

	// 将当前笔触提交到历史列表（标记完成以允许几何缓存）
	m_currentStroke.isComplete = true;
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

// ═══════════════════════════════════════════════════════════
// Catmull-Rom 样条 → 三次 Bezier 控制点转换
// 将四个 Catmull-Rom 控制点 (P0,P1,P2,P3) 转换为以 P1 为起点、
// P2 为终点的三次 Bezier 段，使用 Cardinal 张力 = 0.5。
// ═══════════════════════════════════════════════════════════

struct BezierSegment {
	winrt::Windows::Foundation::Point control1;
	winrt::Windows::Foundation::Point control2;
	winrt::Windows::Foundation::Point end;
};

static BezierSegment CatmullRomToBezier(
	winrt::Windows::Foundation::Point const& p0,
	winrt::Windows::Foundation::Point const& p1,
	winrt::Windows::Foundation::Point const& p2,
	winrt::Windows::Foundation::Point const& p3) {
	// 控制点偏移量：k = (1 - tension) / 6 = 0.5 / 6 = 1 / 12
	constexpr float k = 1.0f / 12.0f;
	return BezierSegment{
		.control1 = { p1.X + k * (p2.X - p0.X), p1.Y + k * (p2.Y - p0.Y) },
		.control2 = { p2.X - k * (p3.X - p1.X), p2.Y - k * (p3.Y - p1.Y) },
		.end = p2
	};
}

// ═══════════════════════════════════════════════════════════
// One Euro Filter 步进实现
// ═══════════════════════════════════════════════════════════

static float ComputeAlpha(float dt, float cutoff) {
	float const tau = 1.0f / (2.0f * 3.14159265358979323846f * cutoff);
	return dt / (dt + tau);
}

winrt::Windows::Foundation::Point OneEuroFilter::Step(
	winrt::Windows::Foundation::Point const& raw,
	std::chrono::steady_clock::time_point const& now) {
	if (!initialized) {
		rawPrev = raw;
		filteredPrev = raw;
		dhatPrev = { 0, 0 };
		tPrev = now;
		initialized = true;
		return raw;
	}

	auto const dt = std::chrono::duration<float>(now - tPrev).count();
	if (dt <= 0.0f) return filteredPrev;

	// 阶段 1：对导数（速度）做低通滤波
	float const dx = (raw.X - rawPrev.X) / dt;
	float const dy = (raw.Y - rawPrev.Y) / dt;
	float const ad = ComputeAlpha(dt, dcutoff);
	float const dxh = dhatPrev.X + ad * (dx - dhatPrev.X);
	float const dyh = dhatPrev.Y + ad * (dy - dhatPrev.Y);

	// 阶段 2：根据速度自适应计算截止频率
	float const speed = std::sqrt(dxh * dxh + dyh * dyh);
	float const cutoff = minCutoff + beta * speed;

	// 阶段 3：对位置信号做低通滤波
	float const a = ComputeAlpha(dt, cutoff);
	float const xh = filteredPrev.X + a * (raw.X - filteredPrev.X);
	float const yh = filteredPrev.Y + a * (raw.Y - filteredPrev.Y);

	// 阶段 4：更新滤波器状态
	rawPrev = raw;
	filteredPrev = { xh, yh };
	dhatPrev = { dxh, dyh };
	tPrev = now;

	return filteredPrev;
}

void MainWindow::DrawStroke(
	mgc::CanvasDrawingSession const& ds,
	PaintStroke const& stroke) {
	if (stroke.points.empty()) return;

	auto const color = stroke.color;
	auto const thickness = stroke.thickness;

	switch (stroke.tool) {

	// ── 画笔模式：Catmull-Rom 样条平滑渲染 ──────────────────
	case DrawingTool::Pen: {
		auto const n = stroke.points.size();
		if (n == 0) break;

		// 单点回退：绘制点状标记
		if (n == 1) {
			auto const& p = stroke.points[0];
			ds.DrawLine(p.X, p.Y, p.X + 0.5f, p.Y + 0.5f, color, thickness);
			break;
		}

		// 双点回退：简单直线
		if (n == 2) {
			auto const& p0 = stroke.points[0];
			auto const& p1 = stroke.points[1];
			ds.DrawLine(p0.X, p0.Y, p1.X, p1.Y, color, thickness);
			break;
		}

		// ≥ 3 点：Catmull-Rom 样条 → CanvasPathBuilder → DrawGeometry
		// 若已缓存，复用几何体（避免每帧重建）
		if (stroke.cachedGeometry) {
			ds.DrawGeometry(stroke.cachedGeometry, color, thickness);
			break;
		}

		auto pathBuilder = mgc::Geometry::CanvasPathBuilder(ds);

		// 虚拟首端控制点：镜像 P1 关于 P0 的对称点
		auto const& P0 = stroke.points[0];
		auto const& P1 = stroke.points[1];
		auto const vBefore = winrt::Windows::Foundation::Point{
			2.0f * P0.X - P1.X, 2.0f * P0.Y - P1.Y
		};

		pathBuilder.BeginFigure({ P0.X, P0.Y });

		// 首段：vBefore → P0 → P1 → P2
		{
			auto seg = CatmullRomToBezier(vBefore, P0, P1, stroke.points[2]);
			pathBuilder.AddCubicBezier(
				{ seg.control1.X, seg.control1.Y },
				{ seg.control2.X, seg.control2.Y },
				{ seg.end.X, seg.end.Y });
		}

		// 中间段：Pi-1 → Pi → Pi+1 → Pi+2  (i = 1 .. n-3)
		for (size_t i = 1; i + 2 < n; ++i) {
			auto seg = CatmullRomToBezier(
				stroke.points[i - 1], stroke.points[i],
				stroke.points[i + 1], stroke.points[i + 2]);
			pathBuilder.AddCubicBezier(
				{ seg.control1.X, seg.control1.Y },
				{ seg.control2.X, seg.control2.Y },
				{ seg.end.X, seg.end.Y });
		}

		// 末段：P{n-3} → P{n-2} → P{n-1} → vAfter
		{
			auto const& Pn2 = stroke.points[n - 2];
			auto const& Pn1 = stroke.points[n - 1];
			auto const vAfter = winrt::Windows::Foundation::Point{
				2.0f * Pn1.X - Pn2.X, 2.0f * Pn1.Y - Pn2.Y
			};
			auto seg = CatmullRomToBezier(
				stroke.points[n - 3], Pn2, Pn1, vAfter);
			pathBuilder.AddCubicBezier(
				{ seg.control1.X, seg.control1.Y },
				{ seg.control2.X, seg.control2.Y },
				{ seg.end.X, seg.end.Y });
		}

		pathBuilder.EndFigure(mgc::Geometry::CanvasFigureLoop::Open);
		auto geometry = mgc::Geometry::CanvasGeometry::CreatePath(pathBuilder);
		// 仅对已完成的笔触缓存几何体——实时预览笔触每帧变化
		if (stroke.isComplete) {
			stroke.cachedGeometry = geometry;
		}
		ds.DrawGeometry(geometry, color, thickness);
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
