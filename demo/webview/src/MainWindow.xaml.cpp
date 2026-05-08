#include "MainWindow.xaml.h"
#include "pch.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <filesystem> // 引入现代 C++ 文件系统库

namespace winrt::webview::implementation {
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::Web::WebView2::Core;

MainWindow::MainWindow() {
	InitializeComponent();

	// 1. 设置窗口标题和初始大小
	this->Title(L"WebView2 WinUI3 Demo");
	this->AppWindow().Resize({ 900, 900 });

	// 2. 释放 WebView2 资源，防止后台进程残留
	this->Closed([this](auto&&, auto&&) {
		if (MyWebView()) {
			MyWebView().Close();
		}
	});

	InitializeWebView();
}

winrt::Windows::Foundation::IAsyncAction MainWindow::InitializeWebView() {
	auto lifetime = get_strong(); // 防止协程期间对象被销毁

	try {
		co_await MyWebView().EnsureCoreWebView2Async();
	}
	catch (winrt::hresult_error const&) {
		co_return; // 如果环境未安装 WebView2 则退出
	}

	// 3. 使用标准库优雅地获取 exe 同级目录下的 web 文件夹
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	auto webDir = std::filesystem::path(exePath).parent_path() / L"web";

	// 4. 映射虚拟主机名 (推荐做法，避免 file:/// 协议的跨域问题)
	MyWebView().CoreWebView2().SetVirtualHostNameToFolderMapping(
		L"appassets",
		webDir.c_str(),
		CoreWebView2HostResourceAccessKind::Allow);

	// 5. 处理 JS 发来的消息 (使用 Lambda 内联，代码更紧凑)
	MyWebView().CoreWebView2().WebMessageReceived(
		[this](auto const& /*sender*/, CoreWebView2WebMessageReceivedEventArgs const& args) {
			auto message = args.TryGetWebMessageAsString();

			// 安全的回复方式，避免 ExecuteScriptAsync 带来的引号语法错误问题
			std::wstring reply = L"[C++ Reply] " + std::wstring(message);
			MyWebView().CoreWebView2().PostWebMessageAsString(reply);
		});

	// 6. 导航到本地 HTML
	MyWebView().CoreWebView2().Navigate(L"https://appassets/index.html");
}

} // namespace winrt::webview::implementation