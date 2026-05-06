#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "pch.h"

namespace winrt::notepad::implementation {
App::App() {
}

void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) {
	m_window = winrt::make<MainWindow>();
	m_window.Activate();
}
} // namespace winrt::notepad::implementation
