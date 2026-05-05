#pragma once

#include <winrt/base.h>
#include "MainWindow.g.h"
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Media.Capture.Frames.h>
#include <winrt/Windows.Media.Capture.h>
#include <winrt/Windows.Media.Core.h>
#include <winrt/Windows.Media.Devices.h>
#include <winrt/Windows.Media.Playback.h>

namespace winrt::camera::implementation {
struct MainWindow : MainWindowT<MainWindow> {
	MainWindow();

	winrt::fire_and_forget GetCameraButton_Click(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);
	winrt::fire_and_forget StartCameraButton_Click(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);
	winrt::fire_and_forget StopCameraButton_Click(
		Windows::Foundation::IInspectable const& sender,
		Microsoft::UI::Xaml::RoutedEventArgs const& args);

private:
	void CloseCamera();
	void SetCameraControlsEnabled(bool isEnabled);
	void ReportError(wchar_t const* operation, winrt::hresult_error const& error);

	winrt::Windows::Devices::Enumeration::DeviceInformationCollection m_devices{ nullptr };
	winrt::Windows::Media::Capture::MediaCapture m_mediaCapture{ nullptr };
	winrt::Windows::Media::Playback::MediaPlayer m_mediaPlayer{ nullptr };
};
} // namespace winrt::camera::implementation

namespace winrt::camera::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
};
} // namespace winrt::camera::factory_implementation
