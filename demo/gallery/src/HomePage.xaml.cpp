#include "pch.h"

#include "HomePage.xaml.h"

#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

namespace winrt::gallery::implementation {
HomePage::HomePage() {
	InitializeComponent();
}
} // namespace winrt::gallery::implementation
