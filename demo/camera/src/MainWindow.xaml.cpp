#include "MainWindow.xaml.h"
#include "pch.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::camera::implementation {
MainWindow::MainWindow() {
	InitializeComponent();
	this->AppWindow().Resize({ 1280, 1120 });
}

void MainWindow::CloseCamera() {
	if (m_mediaPlayer) {
		m_mediaPlayer.Pause();
		m_mediaPlayer.Source(nullptr);
		m_mediaPlayer = nullptr;
	}

	if (m_mediaCapture) {
		m_mediaCapture.Close();
		m_mediaCapture = nullptr;
	}
}

void MainWindow::SetCameraControlsEnabled(bool isEnabled) {
	GetCameraButton().IsEnabled(isEnabled);
	CameraComboBox().IsEnabled(isEnabled);
	StartCameraButton().IsEnabled(isEnabled);
	StopCameraButton().IsEnabled(isEnabled);
}

void MainWindow::ReportError(wchar_t const* operation, winrt::hresult_error const& error) {
	StatusTextBlock().Text(
		winrt::hstring(operation) + L" failed: " + error.message());
}

winrt::fire_and_forget MainWindow::GetCameraButton_Click(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	auto lifetime = get_strong();
	SetCameraControlsEnabled(false);

	try {
		CameraComboBox().Items().Clear();
		auto selector = Windows::Media::Devices::MediaDevice::GetVideoCaptureSelector();
		m_devices = co_await Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(selector);
		for (auto&& device : m_devices) {
			CameraComboBox().Items().Append(box_value(device.Name()));
		}
		if (m_devices.Size()) {
			CameraComboBox().SelectedIndex(0);
		}
		StatusTextBlock().Text(winrt::to_hstring(m_devices.Size()) + L" camera devices found.");
	} catch (winrt::hresult_error const& error) {
		ReportError(L"Get camera devices", error);
	}

	SetCameraControlsEnabled(true);
}

winrt::fire_and_forget MainWindow::StartCameraButton_Click(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	auto lifetime = get_strong();
	auto selectedIndex = CameraComboBox().SelectedIndex();
	if (selectedIndex < 0) {
		StatusTextBlock().Text(L"Please select a camera first.");
		co_return;
	}
	SetCameraControlsEnabled(false);

	try {
		CloseCamera();

		auto device = m_devices.GetAt(selectedIndex);
		Windows::Media::Capture::MediaCaptureInitializationSettings settings{};
		settings.VideoDeviceId(device.Id());
		settings.StreamingCaptureMode(Windows::Media::Capture::StreamingCaptureMode::Video);
		settings.MemoryPreference(Windows::Media::Capture::MediaCaptureMemoryPreference::Cpu);

		m_mediaCapture = winrt::Windows::Media::Capture::MediaCapture();
		co_await m_mediaCapture.InitializeAsync(settings);

		Windows::Media::Capture::Frames::MediaFrameSource source{ nullptr }, fallback{ nullptr };
		for (auto&& pair : m_mediaCapture.FrameSources()) {
			auto&& frameSource = pair.Value();
			if (frameSource.Info().MediaStreamType() == Windows::Media::Capture::MediaStreamType::VideoPreview) {
				source = frameSource;
				break;
			}
			if (frameSource.Info().MediaStreamType() == Windows::Media::Capture::MediaStreamType::VideoRecord) {
				if (!fallback) fallback = frameSource;
			}
		}
		if (!source && fallback) {
			source = fallback;
		}
		if (!source) {
			StatusTextBlock().Text(L"No video preview stream found.");
			CloseCamera();
			SetCameraControlsEnabled(true);
			co_return;
		}
		m_mediaPlayer = winrt::Windows::Media::Playback::MediaPlayer();
		m_mediaPlayer.Source(Windows::Media::Core::MediaSource::CreateFromMediaFrameSource(source));
		CameraPreview().SetMediaPlayer(m_mediaPlayer);
		m_mediaPlayer.Play();
		StatusTextBlock().Text(L"Camera started.");
	} catch (winrt::hresult_error const& error) {
		try {
			CloseCamera();
		} catch (winrt::hresult_error const& closeError) {
			StatusTextBlock().Text(
				winrt::hstring(L"Start camera failed: ") + error.message() +
				L"; cleanup failed: " + closeError.message());
			SetCameraControlsEnabled(true);
			co_return;
		}
		ReportError(L"Start camera", error);
	}

	SetCameraControlsEnabled(true);
}

winrt::fire_and_forget MainWindow::StopCameraButton_Click(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	auto lifetime = get_strong();
	SetCameraControlsEnabled(false);

	try {
		CloseCamera();
		StatusTextBlock().Text(L"Camera stopped.");
	} catch (winrt::hresult_error const& error) {
		ReportError(L"Stop camera", error);
	}

	SetCameraControlsEnabled(true);
	co_return;
}

} // namespace winrt::camera::implementation
