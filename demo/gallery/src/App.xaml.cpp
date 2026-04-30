#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "pch.h"


namespace winrt::gallery::implementation {
App::App() {
#if defined(_DEBUG) && !defined(DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION)
	UnhandledException([](auto&&, Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e) {
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
} // namespace winrt::gallery::implementation
