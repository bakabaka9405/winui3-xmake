#include "pch.h"
#include "NotepadViewModel.h"
#include "NotepadViewModel.g.cpp"

#include <winrt/Microsoft.UI.Xaml.Data.h>

namespace winrt::notepad::implementation
{
    NotepadViewModel::NotepadViewModel()
    {
        m_documentText = L"";
    }

    // -- DocumentText (two-way bound to TextBox) --

    hstring NotepadViewModel::DocumentText()
    {
        return m_documentText;
    }

    void NotepadViewModel::DocumentText(hstring const& value)
    {
        if (m_documentText != value)
        {
            m_documentText = value;

            // Fire PropertyChanged for self + recalibrate computed counts
            m_propertyChanged(
                *this,
                winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(L"DocumentText"));
            UpdateCounts();
        }
    }

    // -- Computed counts --

    int32_t NotepadViewModel::CharCount() const { return m_charCount; }
    int32_t NotepadViewModel::LineCount() const { return m_lineCount; }
    int32_t NotepadViewModel::WordCount() const { return m_wordCount; }

    void NotepadViewModel::UpdateCounts()
    {
        int32_t chars = static_cast<int32_t>(m_documentText.size());

        // Count lines -- TextBox uses \r for newlines, also handle \n
        int32_t lines = 1;
        int32_t words = 0;
        bool inWord = false;
        for (auto ch : m_documentText)
        {
            if (ch == L'\r')
            {
                ++lines;
                inWord = false;
            }
            else if (ch == L'\n')
            {
                ++lines;
                inWord = false;
            }
            else if (ch == L' ' || ch == L'\t')
            {
                inWord = false;
            }
            else
            {
                if (!inWord)
                {
                    ++words;
                    inWord = true;
                }
            }
        }

        // Fire PropertyChanged only for counts that actually changed
        if (m_charCount != chars)
        {
            m_charCount = chars;
            m_propertyChanged(*this,
                winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(L"CharCount"));
        }
        if (m_lineCount != lines)
        {
            m_lineCount = lines;
            m_propertyChanged(*this,
                winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(L"LineCount"));
        }
        if (m_wordCount != words)
        {
            m_wordCount = words;
            m_propertyChanged(*this,
                winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(L"WordCount"));
        }
    }

    // -- Cursor position (updated from code-behind) --

    int32_t NotepadViewModel::CursorLine() const { return m_cursorLine; }
    void NotepadViewModel::CursorLine(int32_t value)
    {
        if (m_cursorLine != value)
        {
            m_cursorLine = value;
            m_propertyChanged(*this,
                winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(L"CursorLine"));
        }
    }

    int32_t NotepadViewModel::CursorColumn() const { return m_cursorColumn; }
    void NotepadViewModel::CursorColumn(int32_t value)
    {
        if (m_cursorColumn != value)
        {
            m_cursorColumn = value;
            m_propertyChanged(*this,
                winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(L"CursorColumn"));
        }
    }

    // -- INotifyPropertyChanged (manual) --

    winrt::event_token NotepadViewModel::PropertyChanged(
        winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void NotepadViewModel::PropertyChanged(winrt::event_token const& token)
    {
        m_propertyChanged.remove(token);
    }
}
