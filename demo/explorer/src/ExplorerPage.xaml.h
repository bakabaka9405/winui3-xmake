#pragma once

#include "ExplorerPage.g.h"
#include "FileItem.g.h"

namespace winrt::explorer::implementation {

struct FileItem : FileItemT<FileItem> {
	FileItem(winrt::hstring const& name, bool isFolder, winrt::hstring const& fullPath)
		: m_name(name), m_isFolder(isFolder), m_fullPath(fullPath) {}

	winrt::hstring Name() const { return m_name; }
	bool IsFolder() const { return m_isFolder; }
	winrt::hstring FullPath() const { return m_fullPath; }
	winrt::hstring Glyph() const { return m_isFolder ? winrt::hstring(L"") : winrt::hstring(L""); }

private:
	winrt::hstring m_name;
	bool m_isFolder;
	winrt::hstring m_fullPath;
};

struct ExplorerPage : ExplorerPageT<ExplorerPage> {
	ExplorerPage();
	void NavigateTo(winrt::hstring const& path);

	void OnBack(winrt::Windows::Foundation::IInspectable const&,
				Microsoft::UI::Xaml::RoutedEventArgs const&);
	void OnForward(winrt::Windows::Foundation::IInspectable const&,
				   Microsoft::UI::Xaml::RoutedEventArgs const&);
	void OnBreadcrumbClicked(Microsoft::UI::Xaml::Controls::BreadcrumbBar const&,
							 Microsoft::UI::Xaml::Controls::BreadcrumbBarItemClickedEventArgs const&);
	void OnItemDoubleTapped(winrt::Windows::Foundation::IInspectable const&,
							Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const&);
	void OnSelectionChanged(winrt::Windows::Foundation::IInspectable const&,
							Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
	void OnViewToggle(winrt::Windows::Foundation::IInspectable const&,
					  Microsoft::UI::Xaml::RoutedEventArgs const&);
	void OnLoaded(winrt::Windows::Foundation::IInspectable const&,
				  Microsoft::UI::Xaml::RoutedEventArgs const&);

private:
	std::wstring m_pendingPath;
	bool m_hasPending{ false };
	bool m_loaded{ false };
	std::wstring m_currentPath;
	std::vector<std::wstring> m_backStack;
	std::vector<std::wstring> m_forwardStack;

	winrt::Windows::Foundation::Collections::IObservableVector<
		winrt::Windows::Foundation::IInspectable>
		m_items{ nullptr };

	void LoadFolder(std::wstring const& path);
	void PushPath(std::wstring const& path);
	void UpdateBreadcrumb(std::wstring const& path);
	void UpdateNavButtons();
};

} // namespace winrt::explorer::implementation

namespace winrt::explorer::factory_implementation {
struct FileItem : FileItemT<FileItem, implementation::FileItem> {};
struct ExplorerPage : ExplorerPageT<ExplorerPage, implementation::ExplorerPage> {};
} // namespace winrt::explorer::factory_implementation
