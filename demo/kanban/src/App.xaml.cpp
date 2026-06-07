#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "pch.h"

#if __has_include("App.xaml.g.cpp")
#include "App.xaml.g.cpp"
#endif

namespace winrt::kanban::implementation {
App::App() {
	InitializeComponent();
}

void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) {
	m_window = winrt::make<MainWindow>();
	m_window.Activate();
}
} // namespace winrt::kanban::implementation
