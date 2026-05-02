#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "pch.h"

namespace winrt::notepad::implementation {
App::App() {
#if defined(_DEBUG) && !defined(DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION)
	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
		FILE* stream;
		freopen_s(&stream, "CONOUT$", "w", stderr);
	}
	UnhandledException([](auto&&, Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e) {
		fwprintf(stderr, L"Unhandled exception: %s\n", e.Message().c_str());
		if (IsDebuggerPresent()) {
			auto message = e.Message();
			(void)message;
			__debugbreak();
		}
	});
#endif
}

void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) {
	m_window = winrt::make<MainWindow>();
	m_window.Activate();
}
} // namespace winrt::notepad::implementation
