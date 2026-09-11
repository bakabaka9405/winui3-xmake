#pragma once

// 单个编译单元在包含本头前定义 WINUI3_NO_MODULE，即可退出模块模式改用文本投影。
// 适用于必须文本引入 STL 的第三方适配层，以及无法消费 MSVC BMI 的 clangd（由 .clangd 注入）。
#if defined(WINUI3_IMPORT_MODULE) && !defined(WINUI3_NO_MODULE)

#include <cstdio>
#include <unknwn.h>
#include <windows.h>

#define WINRT_IMPORT_MODULE
import WINUI3_IMPORT_MODULE;
#else

#include <chrono>
#include <cmath>
#include <hstring.h>
#include <restrictederrorinfo.h>
#include <string>
#include <unknwn.h>
#include <vector>
#include <windows.h>

#undef GetCurrentTime

#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#endif

namespace wf = winrt::Windows::Foundation;
namespace wfc = winrt::Windows::Foundation::Collections;
namespace mux = winrt::Microsoft::UI::Xaml;
namespace muxm = winrt::Microsoft::UI::Xaml::Media;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;
namespace muxp = winrt::Microsoft::UI::Xaml::Controls::Primitives;
namespace mui = winrt::Microsoft::UI;
namespace muic = winrt::Microsoft::UI::Composition;
namespace muxi = winrt::Microsoft::UI::Xaml::Input;
namespace muiw = winrt::Microsoft::UI::Windowing;
