#include "pch.h"

#include "MainWindow.xaml.h"

#include "MarkdownRenderer.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::markdown::implementation {
namespace {
constexpr wchar_t SampleMarkdown[] = LR"(# Native Markdown preview

This preview is built from **WinUI 3 controls** and the _MD4C callback parser_, with $E=mc^2$ inline mathematics.

$$\int_0^1 x^2 \, dx = \frac{1}{3}$$

## Supported content

- Paragraphs with [links](https://github.com/mity/md4c)
- **Strong**, *emphasized*, and `inline code` text
- Nested blocks and ordered lists

1. Edit this document.
2. Select **Refresh preview**.

> The preview never loads HTML or a web view.

---

```cpp
md_parse(source, size, &parser, &state);
```
)";
}

MainWindow::MainWindow() {
	InitializeComponent();

	this->Title(L"Markdown Preview");
	this->AppWindow().Resize({ 1280, 800 });

	auto editor = MarkdownEditor().Editor();
	editor.SetText(SampleMarkdown);
	editor.Modified([weak = get_weak()](auto&&, auto&&) {
		if (auto self = weak.get()) {
			self->RenderPreview();
		}
	});
}

void MainWindow::RefreshPreview_Click(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	RenderPreview();
}

void MainWindow::MarkdownEditor_Loaded(
	wf::IInspectable const&,
	mux::RoutedEventArgs const&) {
	RenderPreview();
	MarkdownEditor().Focus(mux::FocusState::Programmatic);
}

void MainWindow::RenderPreview() {
	auto editor = MarkdownEditor().Editor();
	auto const scale = static_cast<float>(PreviewPanel().XamlRoot().RasterizationScale());
	auto const dpi = 96.0f * scale;
	auto preview = MarkdownRenderer::Render(editor.GetText(editor.Length() + 1), dpi);
	auto children = PreviewPanel().Children();
	children.Clear();
	children.Append(preview);
}
} // namespace winrt::markdown::implementation
