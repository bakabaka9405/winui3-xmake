#include "pch.h"

#include <MddBootstrap.h>

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

// WinAppSDK bootstrap - static initializer
// The XAML-generated code provides the wWinMain entry point
static struct WinAppSdkBootstrap {
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
} g_bootstrap;
