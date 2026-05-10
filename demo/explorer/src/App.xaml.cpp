#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "pch.h"

#if __has_include("App.g.cpp")
#include "App.g.cpp"
#endif

namespace winrt::explorer::implementation {
App::App() {}

void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) {
    m_window = winrt::make<MainWindow>();
    m_window.Activate();
}
} // namespace winrt::explorer::implementation
