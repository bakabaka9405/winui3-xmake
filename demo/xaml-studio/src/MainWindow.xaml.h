#pragma once

#include "MainWindow.g.h"
#include <winrt/Microsoft.Web.WebView2.Core.h>

namespace winrt::xamlstudio::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

    private:
        winrt::Windows::Foundation::IAsyncAction InitializeWebView();
        void OnWebMessageReceived(
            winrt::Microsoft::Web::WebView2::Core::CoreWebView2 const& sender,
            winrt::Microsoft::Web::WebView2::Core::CoreWebView2WebMessageReceivedEventArgs const& args);
        void RenderPreview(winrt::hstring const& xaml);
        void ShowError(winrt::hstring const& message, winrt::hstring const& title = L"Error");
    };
} // namespace winrt::xamlstudio::implementation

namespace winrt::xamlstudio::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
} // namespace winrt::xamlstudio::factory_implementation
