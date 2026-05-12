#pragma once

#include "PaneSplitter.g.h"
#include <winrt/Microsoft.UI.Xaml.Input.h>

namespace winrt::xamlstudio::implementation
{
    struct PaneSplitter : PaneSplitterT<PaneSplitter>
    {
        PaneSplitter();

    private:
        void SetActiveVisual(bool isActive);
        void HandlePointerEntered(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void HandlePointerExited(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void HandlePointerPressed(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void HandlePointerMoved(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void HandlePointerReleased(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void HandlePointerCaptureLost(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

        bool TryBeginResize(winrt::Microsoft::UI::Xaml::Controls::Grid const& layoutRoot);
        void ResetResizeState();

        bool m_isDragging = false;
        double m_dragStartX = 0.0;
        double m_previousColumnStartWidth = 0.0;
        double m_adjacentPaneWidth = 0.0;
        uint32_t m_previousColumnIndex = 0;
        uint32_t m_nextColumnIndex = 0;
    };
} // namespace winrt::xamlstudio::implementation

namespace winrt::xamlstudio::factory_implementation
{
    struct PaneSplitter : PaneSplitterT<PaneSplitter, implementation::PaneSplitter>
    {
    };
} // namespace winrt::xamlstudio::factory_implementation
