#pragma once
#include "NotepadViewModel.g.h"

namespace winrt::notepad::implementation
{
    struct NotepadViewModel : NotepadViewModelT<NotepadViewModel>
    {
        NotepadViewModel();

        hstring DocumentText();
        void DocumentText(hstring const& value);

        // Computed counts -- updated when DocumentText changes
        int32_t CharCount() const;
        int32_t LineCount() const;
        int32_t WordCount() const;

        // Cursor position -- updated from code-behind via x:Bind
        int32_t CursorLine() const;
        void CursorLine(int32_t value);
        int32_t CursorColumn() const;
        void CursorColumn(int32_t value);

        // INotifyPropertyChanged (manual implementation)
        winrt::event_token PropertyChanged(
            winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token);

    private:
        void UpdateCounts();

        hstring m_documentText;
        int32_t m_charCount = 0;
        int32_t m_lineCount = 0;
        int32_t m_wordCount = 0;
        int32_t m_cursorLine = 1;
        int32_t m_cursorColumn = 1;

        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}

namespace winrt::notepad::factory_implementation
{
    struct NotepadViewModel : NotepadViewModelT<NotepadViewModel, implementation::NotepadViewModel>
    {
    };
}
