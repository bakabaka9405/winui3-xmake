#include <windows.h>
#include <unknwn.h>
#include <restrictederrorinfo.h>
#include <hstring.h>
#include <iomanip>
#include <sstream>
#include <format>

#undef GetCurrentTime

#include <MddBootstrap.h>
#include <WindowsAppSDK-VersionInfo.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>

namespace mux = winrt::Microsoft::UI::Xaml;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;

struct bootstrap_winappsdk {
    bootstrap_winappsdk()
    {
        winrt::check_hresult(MddBootstrapInitialize2(
            WINDOWSAPPSDK_RELEASE_MAJORMINOR,
            WINDOWSAPPSDK_RELEASE_VERSION_TAG_W,
            PACKAGE_VERSION{},
            static_cast<MddBootstrapInitializeOptions>(
                MddBootstrapInitializeOptions_OnNoMatch_ShowUI |
                MddBootstrapInitializeOptions_OnError_DebugBreak_IfDebuggerAttached)));
    }

    ~bootstrap_winappsdk()
    {
        MddBootstrapShutdown();
    }
};

struct MainWindow : mux::WindowT<MainWindow> {
    MainWindow()
    {
        auto panel = muxc::StackPanel();
        panel.HorizontalAlignment(mux::HorizontalAlignment::Center);
        panel.VerticalAlignment(mux::VerticalAlignment::Center);

        m_button = muxc::Button();
        m_button.Content(winrt::box_value(L"Click me"));
        m_button.Click({this, &MainWindow::OnClick});

        panel.Children().Append(m_button);
        Content(panel);
        Title(L"xmake + WinUI3 minimal demo");
    }

    void OnClick(winrt::Windows::Foundation::IInspectable const&, mux::RoutedEventArgs const&)
    {
        m_button.Content(winrt::box_value(L"Hello WinUI3"));
    }

    muxc::Button m_button{nullptr};
};

struct App : mux::ApplicationT<App> {
    void OnLaunched(mux::LaunchActivatedEventArgs const&)
    {
        m_window = winrt::make<MainWindow>();
        m_window.Activate();
    }

    mux::Window m_window{nullptr};
};

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    try {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        bootstrap_winappsdk bootstrap;

        mux::Application::Start([](auto&&) { winrt::make<App>(); });

        winrt::uninit_apartment();
        return 0;
    } catch (winrt::hresult_error const& ex) {
        auto details=std::format(L"HRESULT: 0x{0:08X}\n{1}\n\n请先安装 Windows App Runtime 1.6：\nwinget install --id Microsoft.WindowsAppRuntime.1.6 --accept-package-agreements --accept-source-agreements",
            static_cast<uint32_t>(ex.code()),
            ex.message().c_str());
        MessageBoxW(nullptr, details.c_str(), L"WinUI3 demo failed", MB_OK | MB_ICONERROR);
        return 1;
    }
}
