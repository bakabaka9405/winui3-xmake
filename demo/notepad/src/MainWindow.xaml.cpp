#include "MainWindow.xaml.h"
#include "pch.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::notepad::implementation {
MainWindow::MainWindow() {
	// Initialize ViewModel BEFORE InitializeComponent so that x:Bind
	// compiled bindings can resolve the source during binding setup.
	m_viewModel = winrt::make<notepad::implementation::NotepadViewModel>();
	InitializeComponent();
	this->AppWindow().Resize({ 800, 600 });
	this->Title(L"Notepad - x:Bind Demo");
}

notepad::NotepadViewModel MainWindow::ViewModel() const {
	return m_viewModel;
}

void MainWindow::OnFileNew(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	m_viewModel.DocumentText(L"");
}

void MainWindow::OnFileExit(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	this->Close();
}

void MainWindow::OnEditorSelectionChanged(
	wf::IInspectable const& sender,
	mux::RoutedEventArgs const&) {
	auto tb = sender.as<mux::Controls::TextBox>();
	auto pos = tb.SelectionStart();
	auto text = tb.Text();

	// Calculate 1-based line and column from cursor position
	int32_t line = 1;
	int32_t col = 1;
	for (int32_t i = 0; i < pos && i < static_cast<int32_t>(text.size()); ++i) {
		if (text[i] == L'\n') {
			++line;
			col = 1;
		} else {
			++col;
		}
	}
	m_viewModel.CursorLine(line);
	m_viewModel.CursorColumn(col);
}
} // namespace winrt::notepad::implementation
