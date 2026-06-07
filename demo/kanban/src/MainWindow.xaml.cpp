#include "MainWindow.xaml.h"
#include "pch.h"

#include "BoardView.xaml.h"

#include <winrt/Microsoft.UI.Input.h>

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::kanban::implementation {
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Input;

MainWindow::MainWindow() {
	InitializeComponent();
	Root().AddHandler(
		UIElement::PointerPressedEvent(),
		winrt::box_value(PointerEventHandler{ this, &MainWindow::OnRootPointerPressed }),
		true);
	Title(L"Kanban Board");
	AppWindow().Resize({ 1200, 800 });
}

void MainWindow::OnRootPointerPressed(
	winrt::Windows::Foundation::IInspectable const&,
	PointerRoutedEventArgs const& e) {
	winrt::get_self<implementation::BoardView>(Board())->FinishEditIfPointerOutside(e, Root());
}

void MainWindow::OnAddColumn(
	winrt::Windows::Foundation::IInspectable const&,
	Microsoft::UI::Xaml::RoutedEventArgs const&) {
	Board().ViewModel().AddColumn(L"New Column");
}
} // namespace winrt::kanban::implementation
