#include "pch.h"

#include "App.xaml.h"

#include "MainWindow.xaml.h"

namespace winrt::xamlstudio::implementation {
App::App() {
}

void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) {
	m_window = winrt::make<MainWindow>();
	m_window.Activate();
}
} // namespace winrt::xamlstudio::implementation
