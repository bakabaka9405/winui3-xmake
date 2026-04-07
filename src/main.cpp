#include <format>
#include <hstring.h>
#include <iomanip>
#include <restrictederrorinfo.h>
#include <sstream>
#include <unknwn.h>
#include <windows.h>

#undef GetCurrentTime

#include <MddBootstrap.h>
#include <WindowsAppSDK-VersionInfo.h>

#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/base.h>

namespace ux = winrt::Microsoft::UI::Xaml;
namespace uxc = winrt::Microsoft::UI::Xaml::Controls;
namespace uxd = winrt::Microsoft::UI::Xaml::Media;
namespace uxx = winrt::Microsoft::UI::Xaml::XamlTypeInfo;
namespace uxm = winrt::Microsoft::UI::Xaml::Markup;
namespace uxi = winrt::Microsoft::UI::Xaml::Interop;
namespace wuxi = winrt::Windows::UI::Xaml::Interop;

namespace uc = winrt::Microsoft::UI::Composition;
namespace ucs = winrt::Microsoft::UI::Composition::SystemBackdrops;

static void EnableHighDpiSupport() {
	if (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
		return;
	}
	SetProcessDPIAware();
}

struct WinAppSdkBootstrap {
	WinAppSdkBootstrap() {
		winrt::check_hresult(MddBootstrapInitialize2(
			WINDOWSAPPSDK_RELEASE_MAJORMINOR,
			WINDOWSAPPSDK_RELEASE_VERSION_TAG_W,
			PACKAGE_VERSION{},
			static_cast<MddBootstrapInitializeOptions>(
				MddBootstrapInitializeOptions_OnNoMatch_ShowUI | MddBootstrapInitializeOptions_OnError_DebugBreak_IfDebuggerAttached)));
	}

	~WinAppSdkBootstrap() {
		MddBootstrapShutdown();
	}
};

struct MainWindow : ux::WindowT<MainWindow> {
	MainWindow() {
		auto panel = uxc::StackPanel();
		panel.HorizontalAlignment(ux::HorizontalAlignment::Center);
		panel.VerticalAlignment(ux::VerticalAlignment::Center);

		m_textBlock = uxc::TextBlock();
		m_textBlock.Text(L"Hello, xmake + WinUI3!");
		m_textBlock.HorizontalAlignment(ux::HorizontalAlignment::Center);
		m_textBlock.FontSize(24);

		m_button = uxc::Button();
		m_button.Content(winrt::box_value(L"Click me"));
		m_button.Click({ this, &MainWindow::OnClick });
		m_button.HorizontalAlignment(ux::HorizontalAlignment::Center);
		m_button.Margin(ux::ThicknessHelper::FromUniformLength(20));

		panel.Children().Append(m_textBlock);
		panel.Children().Append(m_button);
		Content(panel);

		auto mica = uxd::MicaBackdrop();
		mica.Kind(ucs::MicaKind::BaseAlt);
		SystemBackdrop(mica);

		Title(L"xmake + WinUI3 minimal demo");
	}

	void OnClick(winrt::Windows::Foundation::IInspectable const&,
				 ux::RoutedEventArgs const&) {
		m_button.Content(winrt::box_value(L"Hello WinUI3"));
	}

	uxc::TextBlock m_textBlock{ nullptr };
	uxc::Button m_button{ nullptr };
};

struct App : ux::ApplicationT<App, uxm::IXamlMetadataProvider> {
	void OnLaunched(ux::LaunchActivatedEventArgs const&) {
		Resources().MergedDictionaries().Append(uxc::XamlControlsResources());
		m_window = winrt::make<MainWindow>();
		m_window.Activate();
	}

	uxm::IXamlType GetXamlType(wuxi::TypeName const& type) {
		return provider.GetXamlType(type);
	}
	uxm::IXamlType GetXamlType(winrt::hstring const& fullname) {
		return provider.GetXamlType(fullname);
	}
	winrt::com_array<uxm::XmlnsDefinition> GetXmlnsDefinitions() {
		return provider.GetXmlnsDefinitions();
	}
	ux::Window m_window{ nullptr };
	ux::XamlTypeInfo::XamlControlsXamlMetaDataProvider provider;
};

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	try {
		EnableHighDpiSupport();
		winrt::init_apartment(winrt::apartment_type::single_threaded);
		WinAppSdkBootstrap bootstrap;

		ux::Application::Start([](auto&&) { winrt::make<App>(); });

		winrt::uninit_apartment();
		return 0;
	}
	catch (winrt::hresult_error const& ex) {
		auto details = std::format(L"HRESULT: 0x{0:08X}\n{1}\n\n请先安装 Windows App Runtime 1.7：\nwinget install --id Microsoft.WindowsAppRuntime.1.7 --accept-package-agreements --accept-source-agreements",
								   static_cast<uint32_t>(ex.code()),
								   ex.message().c_str());
		MessageBoxW(nullptr, details.c_str(), L"WinUI3 demo failed", MB_OK | MB_ICONERROR);
		return 1;
	}
}
