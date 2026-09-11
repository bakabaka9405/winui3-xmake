#include "pch.h"

#include "MarkdownRenderer.h"

#include "LatexRenderer.h"

#include <md4c.h>
#include <winrt/Microsoft.UI.Xaml.Documents.h>
#include <winrt/Windows.Data.Html.h>
#include <winrt/Windows.UI.Text.h>

namespace winrt::markdown::implementation {
namespace {
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Documents;

struct InlineStyle final {
	unsigned emphasis{};
	unsigned strong{};
	unsigned code{};
};

struct ListState final {
	bool ordered{};
	unsigned index{};
	bool tight{};
};

struct FormulaState final {
	bool display{}; // 是否为行间公式
	std::string source;
};

struct TextState final {
	RichTextBlock block;
	Paragraph paragraph;
	StackPanel parent;
	unsigned headingLevel{};
	bool implicit{};
};

struct InlineTarget final {
	InlineCollection collection;
	bool paragraph{};
};

std::string DecodeFragment(MD_TEXTTYPE type, std::string_view text) {
	if (type == MD_TEXT_NULLCHAR) {
		std::string decoded;
		for (size_t index = 0; index < text.size(); ++index) {
			decoded += "\xEF\xBF\xBD";
		}
		return decoded;
	}
	if (type != MD_TEXT_ENTITY) {
		return std::string(text);
	}

	auto decoded = winrt::to_string(Windows::Data::Html::HtmlUtilities::ConvertToText(winrt::to_hstring(text)));
	return decoded.empty() ? std::string(text) : decoded;
}

std::string DecodeAttribute(MD_ATTRIBUTE const& attr) {
	if (!attr.text || attr.size == 0) {
		return {};
	}
	if (!attr.substr_types || !attr.substr_offsets) {
		return { attr.text, attr.size };
	}

	std::string decoded;
	for (size_t index = 0; attr.substr_offsets[index] < attr.size; ++index) {
		auto const start = attr.substr_offsets[index];
		auto const end = attr.substr_offsets[index + 1];
		if (start > end || end > attr.size) {
			return { attr.text, attr.size };
		}
		decoded += DecodeFragment(attr.substr_types[index], std::string_view(attr.text + start, end - start));
	}
	return decoded;
};

void ApplyStyle(Inline const& target, InlineStyle const& style) {
	if (style.emphasis != 0) {
		target.FontStyle(Windows::UI::Text::FontStyle::Italic);
	}
	if (style.strong != 0) {
		target.FontWeight(Windows::UI::Text::FontWeights::SemiBold());
	}
	if (style.code != 0) {
		target.FontFamily(Media::FontFamily(L"Cascadia Mono"));
	}
}

void AppendText(std::string_view text, InlineCollection const& destination, InlineStyle const& style) {
	if (text.empty()) return;
	auto run = Run();
	run.Text(winrt::to_hstring(text));
	ApplyStyle(run, style);
	destination.Append(run);
}

class MarkdownRenderState final {
	StackPanel result;
	std::vector<StackPanel> containers;
	std::vector<ListState> lists;
	std::vector<bool> itemImplicit;
	std::vector<TextState> texts;
	std::vector<InlineTarget> inlineTargets;
	std::vector<FormulaState> formulas;
	InlineStyle style;
	std::string currentCodeContent;
	float dpi;
	bool inCodeBlock{};
	bool inHtmlBlock{};
	bool failed{};

	StackPanel const& current_container() const {
		return containers.back();
	}

	InlineCollection const& current_inline_target() const {
		return inlineTargets.back().collection;
	}

	bool current_target_is_paragraph() const {
		return inlineTargets.back().paragraph;
	}

	TextState MakeTextState(StackPanel const& parent, unsigned headingLevel, bool implicit) {
		auto block = RichTextBlock();
		block.TextWrapping(TextWrapping::Wrap);
		block.LineHeight(24);
		if (headingLevel != 0) {
			block.FontSize(headingLevel == 1 ? 30 : headingLevel == 2 ? 24
																	  : 19);
			block.FontWeight(Windows::UI::Text::FontWeights::SemiBold());
			block.Margin({ 0, headingLevel == 1 ? 4.0 : 10.0, 0, 0 });
		}

		auto paragraph = Paragraph();
		block.Blocks().Append(paragraph);
		return { std::move(block), std::move(paragraph), parent, headingLevel, implicit };
	}

	void BeginText(unsigned headingLevel, bool implicit = false) {
		texts.push_back(MakeTextState(current_container(), headingLevel, implicit));
		inlineTargets.push_back({ texts.back().paragraph.Inlines(), true });
	}

	void FlushText() {
		auto& text = texts.back();
		if (text.paragraph.Inlines().Size() != 0) {
			text.block.IsTextSelectionEnabled(true);
			text.parent.Children().Append(text.block);
		}
	}

	void RestartText() {
		auto const parent = texts.back().parent;
		auto const headingLevel = texts.back().headingLevel;
		auto const implicit = texts.back().implicit;
		texts.back() = MakeTextState(parent, headingLevel, implicit);
		inlineTargets.back().collection = texts.back().paragraph.Inlines();
	}

	void EndText() {
		FlushText();
		inlineTargets.pop_back();
		texts.pop_back();
	}

	void FlushImplicitText() {
		if (!texts.empty() && texts.back().implicit) {
			FlushText();
			RestartText();
		}
	}

	void AppendFormula(FormulaState formula) {
		if (formula.display) {
			auto image = TryRenderFormula(formula.source.data(), formula.source.size(), dpi);
			if (!image || !current_target_is_paragraph()) {
				AppendText("$$" + formula.source + "$$", current_inline_target(), style);
				return;
			}
			FlushText();
			RestartText();
			image.HorizontalAlignment(HorizontalAlignment::Center);
			current_container().Children().Append(image);
			return;
		}

		if (auto image = TryRenderFormula(formula.source.data(), static_cast<unsigned>(formula.source.size()), dpi)) {
			auto container = InlineUIContainer();
			container.Child(image);
			current_inline_target().Append(container);
		}
		else {
			AppendText("$" + formula.source + "$", current_inline_target(), style);
		}
	}

public:
	MarkdownRenderState(StackPanel const& destination, float dpi) : result(destination), dpi(dpi) {
		containers.push_back(result);
	}

	bool has_failed() const {
		return failed;
	}

	void fail() {
		failed = true;
	}

	void EnterBlock(MD_BLOCKTYPE type, void* detail) {
		if (type != MD_BLOCK_DOC && type != MD_BLOCK_LI) {
			FlushImplicitText();
		}
		switch (type) {
		case MD_BLOCK_H:
			BeginText(static_cast<MD_BLOCK_H_DETAIL const*>(detail)->level);
			break;
		case MD_BLOCK_P:
			BeginText(0);
			break;
		case MD_BLOCK_CODE:
			currentCodeContent.clear();
			inCodeBlock = true;
			break;
		case MD_BLOCK_HTML:
			inHtmlBlock = true;
			break;
		case MD_BLOCK_UL:
		case MD_BLOCK_OL: {
			auto list = StackPanel();
			list.Spacing(8);
			list.Margin({ 8, 0, 0, 0 });
			current_container().Children().Append(list);
			containers.push_back(list);
			auto const tight = type == MD_BLOCK_OL ? static_cast<MD_BLOCK_OL_DETAIL const*>(detail)->is_tight != 0
												   : static_cast<MD_BLOCK_UL_DETAIL const*>(detail)->is_tight != 0;
			lists.push_back({ type == MD_BLOCK_OL, type == MD_BLOCK_OL ? static_cast<MD_BLOCK_OL_DETAIL const*>(detail)->start : 0, tight });
			break;
		}
		case MD_BLOCK_LI: {
			auto row = Grid();
			row.ColumnSpacing(10);
			auto markerColumn = ColumnDefinition();
			markerColumn.Width(GridLengthHelper::Auto());
			row.ColumnDefinitions().Append(markerColumn);
			auto contentColumn = ColumnDefinition();
			contentColumn.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star));
			row.ColumnDefinitions().Append(contentColumn);

			auto marker = TextBlock();
			marker.MinWidth(22);
			auto& list = lists.back();
			marker.Text(list.ordered ? winrt::hstring(std::to_wstring(list.index++) + L".") : L"•");
			row.Children().Append(marker);

			auto content = StackPanel();
			content.Spacing(8);
			Grid::SetColumn(content, 1);
			row.Children().Append(content);
			current_container().Children().Append(row);
			containers.push_back(content);
			itemImplicit.push_back(list.tight);
			if (list.tight) {
				BeginText(0, true);
			}
			break;
		}
		case MD_BLOCK_QUOTE: {
			auto content = StackPanel();
			content.Spacing(8);
			auto quote = Border();
			quote.Padding({ 14, 2, 0, 2 });
			quote.BorderBrush(Media::SolidColorBrush(Windows::UI::Color{ 255, 0, 120, 212 }));
			quote.BorderThickness({ 3, 0, 0, 0 });
			quote.Child(content);
			current_container().Children().Append(quote);
			containers.push_back(content);
			break;
		}
		case MD_BLOCK_HR: {
			auto rule = Border();
			rule.Height(1);
			rule.Margin({ 0, 6, 0, 6 });
			rule.Background(Media::SolidColorBrush(Windows::UI::Color{ 70, 128, 128, 128 }));
			current_container().Children().Append(rule);
			break;
		}
		default:
			break;
		}
	}

	void LeaveBlock(MD_BLOCKTYPE type) {
		switch (type) {
		case MD_BLOCK_H:
		case MD_BLOCK_P:
			EndText();
			break;
		case MD_BLOCK_CODE: {
			auto cb = TextBlock();
			cb.FontFamily(Media::FontFamily(L"Cascadia Mono"));
			cb.FontSize(13);
			cb.IsTextSelectionEnabled(true);
			cb.Text(winrt::to_hstring(currentCodeContent));
			cb.TextWrapping(TextWrapping::NoWrap);
			auto scroller = ScrollViewer();
			scroller.HorizontalScrollBarVisibility(ScrollBarVisibility::Auto);
			scroller.HorizontalScrollMode(ScrollMode::Enabled);
			scroller.VerticalScrollBarVisibility(ScrollBarVisibility::Disabled);
			scroller.Content(cb);
			auto border = Border();
			border.Padding({ 14, 12, 14, 12 });
			border.CornerRadius({ 6, 6, 6, 6 });
			border.Background(Media::SolidColorBrush(Windows::UI::Color{ 18, 128, 128, 128 }));
			border.Child(scroller);
			current_container().Children().Append(border);
			inCodeBlock = false;
			break;
		}
		case MD_BLOCK_HTML:
			inHtmlBlock = false;
			break;
		case MD_BLOCK_LI:
			if (itemImplicit.back()) {
				EndText();
			}
			itemImplicit.pop_back();
			containers.pop_back();
			break;
		case MD_BLOCK_QUOTE:
			containers.pop_back();
			break;
		case MD_BLOCK_UL:
		case MD_BLOCK_OL:
			lists.pop_back();
			containers.pop_back();
			break;
		default:
			break;
		}
	}

	void EnterSpan(MD_SPANTYPE type, void* detail) {
		switch (type) {
		case MD_SPAN_EM:
			++style.emphasis;
			break;
		case MD_SPAN_STRONG:
			++style.strong;
			break;
		case MD_SPAN_CODE:
			++style.code;
			break;
		case MD_SPAN_A: {
			auto const* link = static_cast<MD_SPAN_A_DETAIL const*>(detail);
			try {
				auto hyperlink = Hyperlink();
				hyperlink.NavigateUri(wf::Uri(winrt::to_hstring(DecodeAttribute(link->href))));
				current_inline_target().Append(hyperlink);
				inlineTargets.push_back({ hyperlink.Inlines(), false });
			}
			catch (winrt::hresult_error const&) {
				inlineTargets.push_back({ current_inline_target(), false });
			}
			break;
		}
		case MD_SPAN_LATEXMATH:
			formulas.push_back({ false, {} });
			break;
		case MD_SPAN_LATEXMATH_DISPLAY:
			formulas.push_back({ true, {} });
			break;
		default:
			break;
		}
	}

	void LeaveSpan(MD_SPANTYPE type) {
		switch (type) {
		case MD_SPAN_EM:
			--style.emphasis;
			break;
		case MD_SPAN_STRONG:
			--style.strong;
			break;
		case MD_SPAN_CODE:
			--style.code;
			break;
		case MD_SPAN_A:
			inlineTargets.pop_back();
			break;
		case MD_SPAN_LATEXMATH:
		case MD_SPAN_LATEXMATH_DISPLAY: {
			auto formula = std::move(formulas.back());
			formulas.pop_back();
			AppendFormula(std::move(formula));
			break;
		}
		default:
			break;
		}
	}

	void Text(MD_TEXTTYPE type, MD_CHAR const* text, MD_SIZE size) {
		auto const value = std::string_view(text, size);
		if (type == MD_TEXT_LATEXMATH) {
			formulas.back().source.append(value);
			return;
		}
		if (inHtmlBlock || type == MD_TEXT_HTML) {
			return;
		}
		if (inCodeBlock) {
			currentCodeContent += DecodeFragment(type, value);
			return;
		}
		switch (type) {
		case MD_TEXT_SOFTBR:
			AppendText(" ", current_inline_target(), style);
			break;
		case MD_TEXT_BR:
			current_inline_target().Append(LineBreak());
			break;
		default:
			AppendText(DecodeFragment(type, value), current_inline_target(), style);
			break;
		}
	}
};

int EnterBlock(MD_BLOCKTYPE type, void* detail, void* userdata) {
	auto* state = static_cast<MarkdownRenderState*>(userdata);
	try {
		state->EnterBlock(type, detail);
		return 0;
	}
	catch (...) {
		state->fail();
		return 1;
	}
}

int LeaveBlock(MD_BLOCKTYPE type, void* detail, void* userdata) {
	auto* state = static_cast<MarkdownRenderState*>(userdata);
	try {
		state->LeaveBlock(type);
		return 0;
	}
	catch (...) {
		state->fail();
		return 1;
	}
}

int EnterSpan(MD_SPANTYPE type, void* detail, void* userdata) {
	auto* state = static_cast<MarkdownRenderState*>(userdata);
	try {
		state->EnterSpan(type, detail);
		return 0;
	}
	catch (...) {
		state->fail();
		return 1;
	}
}

int LeaveSpan(MD_SPANTYPE type, void* detail, void* userdata) {
	auto* state = static_cast<MarkdownRenderState*>(userdata);
	try {
		state->LeaveSpan(type);
		return 0;
	}
	catch (...) {
		state->fail();
		return 1;
	}
}

int Text(MD_TEXTTYPE type, MD_CHAR const* text, MD_SIZE size, void* userdata) {
	auto* state = static_cast<MarkdownRenderState*>(userdata);
	try {
		state->Text(type, text, size);
		return 0;
	}
	catch (...) {
		state->fail();
		return 1;
	}
}
} // namespace

StackPanel MarkdownRenderer::Render(winrt::hstring const& content, float dpi) {
	auto result = StackPanel();
	result.Spacing(14);

	auto utf8 = winrt::to_string(content);
	MarkdownRenderState state(result, dpi);
	MD_PARSER parser{};
	parser.flags = MD_FLAG_LATEXMATHSPANS;
	parser.enter_block = EnterBlock;
	parser.leave_block = LeaveBlock;
	parser.enter_span = EnterSpan;
	parser.leave_span = LeaveSpan;
	parser.text = Text;
	if (md_parse(utf8.data(), utf8.size(), &parser, &state) != 0 || state.has_failed()) {
		result.Children().Clear();
		auto error = TextBlock();
		error.Text(L"Markdown could not be parsed.");
		result.Children().Append(error);
	}
	return result;
}
} // namespace winrt::markdown::implementation
