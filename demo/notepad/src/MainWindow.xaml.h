#pragma once
#include "MainWindow.g.h"
#include "NotepadViewModel.h"

namespace winrt::notepad::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        notepad::NotepadViewModel ViewModel() const;

        void OnFileNew(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnFileExit(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnEditorSelectionChanged(
            Windows::Foundation::IInspectable const& sender,
            Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        notepad::NotepadViewModel m_viewModel{ nullptr };
    };
}

namespace winrt::notepad::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
