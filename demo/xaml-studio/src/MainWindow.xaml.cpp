#include "pch.h"

#include "MainWindow.xaml.h"

#include <filesystem>

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::xamlstudio::implementation {
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::Web::WebView2::Core;

MainWindow::MainWindow() {
	InitializeComponent();

	// Set window title and size
	this->Title(L"XAML Studio");
	this->AppWindow().Resize({ 1200, 800 });

	// Close WebView2 on window close to prevent lingering processes
	this->Closed([this](auto&&, auto&&) {
		if (EditorWebView()) {
			EditorWebView().Close();
		}
	});

	InitializeWebView();
}

winrt::Windows::Foundation::IAsyncAction MainWindow::InitializeWebView() {
	auto lifetime = get_strong();

	// Asynchronously initialize WebView2 environment
	try {
		co_await EditorWebView().EnsureCoreWebView2Async();
	}
	catch (winrt::hresult_error const&) {
		ShowError(L"Failed to initialize WebView2 runtime.", L"WebView2 Error");
		co_return;
	}

	// Resolve web/ directory relative to the executable
	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	auto webDir = std::filesystem::path(std::wstring(exePath)).parent_path() / L"web";

	// Map virtual host to web/ directory (avoids file:/// cross-origin issues)
	EditorWebView().CoreWebView2().SetVirtualHostNameToFolderMapping(
		L"appassets",
		webDir.c_str(),
		CoreWebView2HostResourceAccessKind::Allow);

	// Register JavaScript-to-C++ message bridge
	EditorWebView().CoreWebView2().WebMessageReceived(
		[this](CoreWebView2 const& sender, CoreWebView2WebMessageReceivedEventArgs const& args) {
			OnWebMessageReceived(sender, args);
		});

	// Navigate to the editor HTML page
	EditorWebView().CoreWebView2().Navigate(L"https://appassets/index.html");
}

void MainWindow::OnWebMessageReceived(
	CoreWebView2 const& /*sender*/,
	CoreWebView2WebMessageReceivedEventArgs const& args) {
	auto message = args.TryGetWebMessageAsString();
	if (message.empty()) {
		return;
	}

	// Check for error messages from editor
	std::wstring msgStr(message);
	if (msgStr.find(L"__ERROR__") == 0) {
		ShowError(message, L"Editor Load Error");
		return;
	}

	// Route XAML content to preview
	RenderPreview(message);
}

void MainWindow::RenderPreview(winrt::hstring const& xaml) {
	// Clear previous preview content
	if (PreviewHost()) {
		PreviewHost().Child(nullptr);
	}
	if (EmptyStateText()) {
		EmptyStateText().Visibility(Visibility::Collapsed);
	}
	if (ErrorInfoBar()) {
		ErrorInfoBar().IsOpen(false);
	}

	// Handle empty or whitespace-only XAML
	std::wstring xamlStr(xaml);
	bool isWhitespace = true;
	for (auto ch : xamlStr) {
		if (!iswspace(static_cast<wint_t>(ch))) {
			isWhitespace = false;
			break;
		}
	}
	if (xamlStr.empty() || isWhitespace) {
		if (EmptyStateText()) {
			EmptyStateText().Text(L"Enter XAML to preview");
			EmptyStateText().Visibility(Visibility::Visible);
		}
		return;
	}

	// Attempt to load XAML via XamlReader
	try {
		auto loaded = Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml);

		// Try to attach as UIElement to preview host
		auto element = loaded.try_as<Microsoft::UI::Xaml::UIElement>();
		if (element) {
			if (PreviewHost()) {
				PreviewHost().Child(element);
			}
		}
		else {
			ShowError(L"Loaded XAML object cannot be displayed as a UI element. "
					  "Ensure your root element is a UIElement (e.g. Page, Grid, StackPanel).",
					  L"Display Error");
		}
	}
	catch (winrt::hresult_error const& e) {
		// XAML parse error — show in-page, do not crash
		std::wstring errorMsg = L"XAML Parse Error: " + std::wstring(e.message());
		ShowError(winrt::hstring(errorMsg), L"Parse Error");
	}
	catch (...) {
		ShowError(L"An unknown error occurred while parsing XAML.", L"Parse Error");
	}
}

void MainWindow::ShowError(winrt::hstring const& message, winrt::hstring const& title) {
	if (ErrorInfoBar()) {
		ErrorInfoBar().Title(title);
		ErrorInfoBar().Message(message);
		ErrorInfoBar().IsOpen(true);
	}
	// Hide empty state when showing error
	if (EmptyStateText()) {
		EmptyStateText().Visibility(Visibility::Collapsed);
	}
}

} // namespace winrt::xamlstudio::implementation
