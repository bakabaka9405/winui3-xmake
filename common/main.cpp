#include "pch.h"

#include <MddBootstrap.h>
#include <cstdio>
#include <exception>

// WinAppSDK 2.0+ removed <WindowsAppSDK-VersionInfo.h> from the NuGet packages.
// Define the version macros manually (SemVer scheme, stable GA release).
#define WINDOWSAPPSDK_RELEASE_MAJORMINOR    0x00020000   // Major=2, Minor=0
#define WINDOWSAPPSDK_RELEASE_VERSION_TAG_W L""           // No pre-release tag for stable
#define WINDOWSAPPSDK_RUNTIME_VERSION_UINT64 0

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

#ifndef WINUI3_APP_NAMESPACE
#error WINUI3_APP_NAMESPACE must be defined by the winui3.app xmake rule.
#endif

namespace {

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

void AttachDebugConsole() {
#if defined(_DEBUG) && !defined(DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION)
	static bool attached = false;
	if (!attached && AttachConsole(ATTACH_PARENT_PROCESS)) {
		FILE* stream{};
		freopen_s(&stream, "CONOUT$", "w", stderr);
		attached = true;
	}
#endif
}

void BreakIfDebuggerPresent() {
#if defined(_DEBUG) && !defined(DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION)
	if (IsDebuggerPresent()) {
		__debugbreak();
	}
#endif
}

void RegisterUnhandledExceptionHandler(mux::Application const& app) {
#if defined(_DEBUG) && !defined(DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION)
	AttachDebugConsole();

	app.UnhandledException([](auto&&, mux::UnhandledExceptionEventArgs const& e) {
		fwprintf(stderr, L"Unhandled exception: %s\n", e.Message().c_str());
		BreakIfDebuggerPresent();
	});
#else
	(void)app;
#endif
}

void ReportStartupException(winrt::hresult_error const& e) {
#if defined(_DEBUG) && !defined(DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION)
	AttachDebugConsole();
	fwprintf(stderr, L"Unhandled exception during startup: %s\n", e.message().c_str());
	BreakIfDebuggerPresent();
#else
	(void)e;
#endif
}

void ReportStartupException(std::exception const& e) {
#if defined(_DEBUG) && !defined(DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION)
	AttachDebugConsole();
	fprintf(stderr, "Unhandled exception during startup: %s\n", e.what());
	BreakIfDebuggerPresent();
#else
	(void)e;
#endif
}

void ReportUnknownStartupException() {
#if defined(_DEBUG) && !defined(DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION)
	AttachDebugConsole();
	fwprintf(stderr, L"Unhandled exception during startup.\n");
	BreakIfDebuggerPresent();
#endif
}

} // namespace

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
	winrt::init_apartment(winrt::apartment_type::single_threaded);
	WinAppSdkBootstrap bootstrap;

	mux::Application::Start([](auto&&) {
		try {
			auto app = winrt::make<winrt::WINUI3_APP_NAMESPACE::implementation::App>();
			RegisterUnhandledExceptionHandler(app);
		} catch (winrt::hresult_error const& e) {
			ReportStartupException(e);
			throw;
		} catch (std::exception const& e) {
			ReportStartupException(e);
			throw;
		} catch (...) {
			ReportUnknownStartupException();
			throw;
		}
	});

	return 0;
}
