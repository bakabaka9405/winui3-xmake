#pragma once

#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>

#include "MainWindow.g.h"
#include "PointerFilter.h"

namespace mgc = winrt::Microsoft::Graphics::Canvas;
namespace mgcux = winrt::Microsoft::Graphics::Canvas::UI::Xaml;

namespace winrt::paint::implementation {

enum class DrawingTool {
	Pen,
	Line,
	Rectangle,
	Ellipse,
};

struct PaintStroke {
	std::vector<winrt::Windows::Foundation::Point> points;
	DrawingTool tool;
	winrt::Windows::UI::Color color;
	float thickness;
	bool isComplete = false;
	// 仅完成后的笔触可缓存几何体；实时预览路径每帧都会变化。
	mutable mgc::Geometry::CanvasGeometry cachedGeometry{ nullptr };
};

struct MainWindow : MainWindowT<MainWindow> {
	MainWindow();

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

	void ToolButton_Checked(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void ToolButton_Unchecked(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void BrushColorPicker_ColorChanged(
		winrt::Microsoft::UI::Xaml::Controls::ColorPicker const& sender,
		winrt::Microsoft::UI::Xaml::Controls::ColorChangedEventArgs const& args);

	void PresetColor_Click(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void UndoButton_Click(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

	void ClearButton_Click(
		winrt::Windows::Foundation::IInspectable const& sender,
		winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

private:
	DrawingTool GetActiveTool();
	void UpdateToolButtonStates(DrawingTool tool);
	void UpdateColorPreview();
	void BuildPresetColors();
	void DrawStroke(mgc::CanvasDrawingSession const& ds, PaintStroke const& stroke);
	void UpdateButtonStates();
	winrt::Windows::Foundation::Point GetCanvasPoint(
		winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

	std::vector<PaintStroke> m_strokes;
	PaintStroke m_currentStroke{};
	bool m_isDrawing = false;
	DrawingTool m_activeTool = DrawingTool::Pen;
	bool m_isUpdatingTools = false;
	OneEuroFilter m_pointerFilter;

	// XAML 加载期间可能触发事件，控件完全初始化前忽略这些回调。
	bool m_isInitialized = false;
};

} // namespace winrt::paint::implementation

namespace winrt::paint::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
};
} // namespace winrt::paint::factory_implementation
