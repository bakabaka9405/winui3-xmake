#pragma once

// 仅引入返回类型所需的投影头。在模块模式下该头内部受 WINRT_IMPORT_MODULE 保护，为空操作。
#include <winrt/Microsoft.UI.Xaml.Controls.h>

namespace winrt::markdown::implementation {
// 将 LaTeX 公式渲染为 WinUI 图像；解析或渲染失败时返回 nullptr。
winrt::Microsoft::UI::Xaml::Controls::Image TryRenderFormula(char const* formula, unsigned length, float dpi);
} // namespace winrt::markdown::implementation
