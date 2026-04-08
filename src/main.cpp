#include "pch.h"

#include <MddBootstrap.h>
#include <WindowsAppSDK-VersionInfo.h>

#include "App.xaml.h"

#if __has_include("module.g.cpp")
#include "module.g.cpp"
#endif

#if __has_include("XamlLibMetadataProvider.g.cpp")
#include "XamlLibMetadataProvider.g.cpp"
#endif

#if __has_include("XamlTypeInfo.Impl.g.cpp")
#include "XamlTypeInfo.Impl.g.cpp"
#endif

#if __has_include("XamlTypeInfo.g.cpp")
#include "XamlTypeInfo.g.cpp"
#endif

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

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	EnableHighDpiSupport();

	winrt::init_apartment(winrt::apartment_type::single_threaded);
	WinAppSdkBootstrap bootstrap;

	mux::Application::Start([](auto&&) {
		winrt::make<winrt::xmake_demo::implementation::App>();
	});

	winrt::uninit_apartment();
	return 0;
}
