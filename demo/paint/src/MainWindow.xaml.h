#pragma once

#include <chrono>

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
	bool isComplete = false;                              // 笔触是否已提交——决定是否缓存几何体
	mutable mgc::Geometry::CanvasGeometry cachedGeometry{ nullptr };
};

// One Euro Filter — 自适应低通滤波，消除手抖噪声
// 参考：Casiez, Roussel, Vogel (CHI 2012)
// 原理：根据笔触速度动态调整截止频率——慢速强滤波，快速弱滤波
struct OneEuroFilter {
	float minCutoff = 1.0f;                             // 最低截止频率 (Hz)
	float beta = 0.007f;                                 // 速度系数
	float dcutoff = 1.0f;                                // 导数滤波截止频率 (Hz)

	bool initialized = false;
	winrt::Windows::Foundation::Point rawPrev{};
	winrt::Windows::Foundation::Point filteredPrev{};
	winrt::Windows::Foundation::Point dhatPrev{};
	std::chrono::steady_clock::time_point tPrev{};

	void Reset() { initialized = false; }

	winrt::Windows::Foundation::Point Step(
		winrt::Windows::Foundation::Point const& raw,
		std::chrono::steady_clock::time_point const& now);
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

	// ── 工具切换事件 ────────────────────────────────────────
	void ToolButton_Checked(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void ToolButton_Unchecked(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	// ── 取色器事件 ──────────────────────────────────────────
	void BrushColorPicker_ColorChanged(
		winrt::Microsoft::UI::Xaml::Controls::ColorPicker const& sender,
		winrt::Microsoft::UI::Xaml::Controls::ColorChangedEventArgs const& args);

	void PresetColor_Click(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	// ── 操作按钮事件 ────────────────────────────────────────
	void UndoButton_Click(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void ClearButton_Click(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

private:
	// ── 辅助函数 ────────────────────────────────────────────
	DrawingTool GetActiveTool();
	void UpdateToolButtonStates(DrawingTool tool);
	void UpdateColorPreview();
	void BuildPresetColors();
	void DrawStroke(mgc::CanvasDrawingSession const& ds, PaintStroke const& stroke);
	void UpdateButtonStates();
	winrt::Windows::Foundation::Point GetCanvasPoint(
		winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

	// ── 绘图状态 ────────────────────────────────────────────
	std::vector<PaintStroke> m_strokes;
	PaintStroke m_currentStroke{};
	bool m_isDrawing = false;
	DrawingTool m_activeTool = DrawingTool::Pen;
	bool m_isUpdatingTools = false;
	OneEuroFilter m_pointerFilter;

	// 初始化守卫：防止 XAML 加载期间事件访问尚未就绪的控件
	bool m_isInitialized = false;
};

} // namespace winrt::paint::implementation

namespace winrt::paint::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
};
} // namespace winrt::paint::factory_implementation
