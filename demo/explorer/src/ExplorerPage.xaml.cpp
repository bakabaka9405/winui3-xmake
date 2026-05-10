#include "ExplorerPage.xaml.h"
#include "pch.h"
#include <sstream>

#if __has_include("ExplorerPage.g.cpp")
#include "ExplorerPage.g.cpp"
#endif
#if __has_include("FileItem.g.cpp")
#include "FileItem.g.cpp"
#endif

namespace winrt::explorer::implementation {

ExplorerPage::ExplorerPage() {
	m_items = winrt::single_threaded_observable_vector<wf::IInspectable>();
	InitializeComponent();
	FileGridView().ItemsSource(m_items);
	FileListView().ItemsSource(m_items);
}

void ExplorerPage::NavigateTo(hstring const& path) {
	if (!m_loaded) {
		m_pendingPath = std::wstring(path);
		m_hasPending = true;
		return;
	}
	PushPath(std::wstring(path));
	LoadFolder(std::wstring(path));
}

void ExplorerPage::OnLoaded(wf::IInspectable const&, mux::RoutedEventArgs const&) {
	m_loaded = true;
	if (m_hasPending) {
		m_hasPending = false;
		PushPath(m_pendingPath);
		LoadFolder(m_pendingPath);
		m_pendingPath.clear();
	}
}

void ExplorerPage::PushPath(std::wstring const& path) {
	if (!m_currentPath.empty())
		m_backStack.push_back(m_currentPath);
	m_forwardStack.clear();
	m_currentPath = path;
}

void ExplorerPage::LoadFolder(std::wstring const& path) {
	m_items.Clear();

	// Special case: empty path = list logical drives
	if (path.empty()) {
		DWORD mask = GetLogicalDrives();
		for (int i = 0; i < 26; ++i) {
			if (!(mask & (1 << i))) continue;
			wchar_t letter[4] = { (wchar_t)(L'A' + i), L':', L'\\', L'\0' };
			auto item = winrt::make<FileItem>(hstring(std::wstring(letter, 2) + L":"), true, hstring(letter));
			m_items.Append(item);
		}
		UpdateBreadcrumb(path);
		UpdateNavButtons();
		StatusText().Text(std::to_wstring(m_items.Size()) + L" drives");
		return;
	}

	std::wstring pattern = path;
	if (!pattern.empty() && pattern.back() != L'\\') pattern += L'\\';
	pattern += L'*';

	WIN32_FIND_DATAW fd{};
	HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) {
		StatusText().Text(L"Cannot open folder");
		return;
	}

	// Two passes: folders first, then files
	std::vector<WIN32_FIND_DATAW> folders, files;
	do {
		std::wstring name = fd.cFileName;
		if (name == L"." || name == L"..") continue;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) continue;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			folders.push_back(fd);
		else
			files.push_back(fd);
	} while (FindNextFileW(h, &fd));
	FindClose(h);

	std::wstring base = path;
	if (!base.empty() && base.back() != L'\\') base += L'\\';

	for (auto& d : folders) {
		auto item = winrt::make<FileItem>(
			hstring(d.cFileName), true, hstring(base + d.cFileName));
		m_items.Append(item);
	}
	for (auto& f : files) {
		auto item = winrt::make<FileItem>(
			hstring(f.cFileName), false, hstring(base + f.cFileName));
		m_items.Append(item);
	}

	UpdateBreadcrumb(path);
	UpdateNavButtons();

	auto total = (uint32_t)(folders.size() + files.size());
	StatusText().Text(std::to_wstring(total) + L" items  (" + std::to_wstring(folders.size()) + L" folders, " + std::to_wstring(files.size()) + L" files)");
}

void ExplorerPage::UpdateBreadcrumb(std::wstring const& path) {
	auto vec = winrt::single_threaded_observable_vector<wf::IInspectable>();
	if (path.empty()) {
		vec.Append(winrt::box_value(hstring(L"This PC")));
	}
	else {
		std::wstringstream ss(path);
		std::wstring seg;
		while (std::getline(ss, seg, L'\\'))
			if (!seg.empty()) vec.Append(winrt::box_value(hstring(seg)));
	}
	Breadcrumb().ItemsSource(vec);
}

void ExplorerPage::UpdateNavButtons() {
	BackButton().IsEnabled(!m_backStack.empty());
	ForwardButton().IsEnabled(!m_forwardStack.empty());
}

void ExplorerPage::OnBack(wf::IInspectable const&, mux::RoutedEventArgs const&) {
	if (m_backStack.empty()) return;
	m_forwardStack.push_back(m_currentPath);
	m_currentPath = m_backStack.back();
	m_backStack.pop_back();
	LoadFolder(m_currentPath);
	UpdateNavButtons();
}

void ExplorerPage::OnForward(wf::IInspectable const&, mux::RoutedEventArgs const&) {
	if (m_forwardStack.empty()) return;
	m_backStack.push_back(m_currentPath);
	m_currentPath = m_forwardStack.back();
	m_forwardStack.pop_back();
	LoadFolder(m_currentPath);
	UpdateNavButtons();
}

void ExplorerPage::OnBreadcrumbClicked(
	muxc::BreadcrumbBar const&,
	muxc::BreadcrumbBarItemClickedEventArgs const& args) {
	auto src = Breadcrumb().ItemsSource().as<wfc::IVector<wf::IInspectable>>();
	auto first = winrt::unbox_value<hstring>(src.GetAt(0));
	if (first == L"This PC") {
		PushPath(L"");
		LoadFolder(L"");
		return;
	}
	std::wstring path;
	for (uint32_t i = 0; i <= (uint32_t)args.Index(); ++i) {
		auto seg = winrt::unbox_value<hstring>(src.GetAt(i));
		path += std::wstring(seg) + L"\\";
	}
	if (path.size() > 3 && path.back() == L'\\')
		path.pop_back();
	PushPath(path);
	LoadFolder(path);
}

void ExplorerPage::OnItemDoubleTapped(
	wf::IInspectable const&,
	muxi::DoubleTappedRoutedEventArgs const&) {
	wf::IInspectable sel = FileGridView().Visibility() == mux::Visibility::Visible
							   ? FileGridView().SelectedItem()
							   : FileListView().SelectedItem();
	auto item = sel.try_as<explorer::FileItem>();
	if (!item) return;
	if (item.IsFolder()) {
		PushPath(std::wstring(item.FullPath()));
		LoadFolder(std::wstring(item.FullPath()));
	}
	else {
		auto path = item.FullPath();
		[](hstring p) -> wf::IAsyncAction {
			auto file = co_await winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(p);
			co_await winrt::Windows::System::Launcher::LaunchFileAsync(file);
		}(path);
	}
}

void ExplorerPage::OnSelectionChanged(
	wf::IInspectable const&,
	muxc::SelectionChangedEventArgs const&) {
	auto total = m_items.Size();
	int32_t sel = FileGridView().Visibility() == mux::Visibility::Visible
					  ? FileGridView().SelectedIndex()
					  : FileListView().SelectedIndex();
	if (sel >= 0) {
		StatusText().Text(std::to_wstring(total) + L" items  — 1 selected");
	}
}

void ExplorerPage::OnViewToggle(wf::IInspectable const&, mux::RoutedEventArgs const&) {
	bool showGrid = FileListView().Visibility() == mux::Visibility::Visible;
	FileGridView().Visibility(showGrid ? mux::Visibility::Visible : mux::Visibility::Collapsed);
	FileListView().Visibility(showGrid ? mux::Visibility::Collapsed : mux::Visibility::Visible);
	// &#xF0E2; = grid view glyph, &#xE8FD; = list view glyph (Segoe MDL2)
	ViewToggleIcon().Glyph(showGrid ? L"" : L"");
}

} // namespace winrt::explorer::implementation
