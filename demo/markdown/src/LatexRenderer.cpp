// MicroTeX 头会文本引入 STL，与聚合模块的 import std 冲突。
// 在包含 pch.h 前声明本编译单元退出模块模式，使 pch.h 走文本投影分支。
#define WINUI3_NO_MODULE
#include "pch.h"

#include "LatexRenderer.h"

#include <filesystem>
#include <latex.h>
#include <limits>
#include <memory>
#include <render.h>
#include <string>
#include <vector>
#include <winrt/Microsoft.Graphics.Canvas.Brushes.h>
#include <winrt/Microsoft.Graphics.Canvas.Geometry.h>
#include <winrt/Microsoft.Graphics.Canvas.Text.h>
#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Windows.UI.Text.h>
#include <winrt/Windows.UI.ViewManagement.h>

namespace {

using winrt::Windows::Foundation::Numerics::float3x2;

namespace canvas = winrt::Microsoft::Graphics::Canvas;

struct MicroTeXContext final {
	std::filesystem::path resourceRoot;
	bool latexInitialized{};

	MicroTeXContext() {
		auto executable = std::wstring(MAX_PATH, L'\0');
		auto length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
		if (length == 0 || length == executable.size()) {
			throw winrt::hresult_error(E_FAIL, L"无法确定应用程序目录。");
		}

		resourceRoot = std::filesystem::path(executable).parent_path() / L"res";
	}

	~MicroTeXContext() {
		if (latexInitialized) {
			tex::LaTeX::release();
		}
	}
};

MicroTeXContext& getMicroTeXContext() {
	static MicroTeXContext ctx;
	return ctx;
}

std::wstring getFontFaceFamilyName(canvas::Text::CanvasFontFace const& face) {
	auto const names = face.FamilyNames();
	if (names.Size() == 0) {
		throw winrt::hresult_invalid_argument(L"字体文件未提供字体族。");
	}
	if (names.HasKey(L"en-us")) {
		return std::wstring(names.Lookup(L"en-us"));
	}
	return std::wstring(names.First().Current().Value());
}

class FontWrapper final : public tex::Font {
	std::wstring family;
	winrt::hstring fileUri;
	int style;
	float size;
	canvas::Text::CanvasFontFace face;

public:
	FontWrapper(std::wstring family, winrt::hstring fileUri, int style, float size, canvas::Text::CanvasFontFace face)
		: family(std::move(family)), fileUri(std::move(fileUri)), style(style), size(size), face(std::move(face)) {}

	float getSize() const override {
		return size;
	}

	tex::sptr<tex::Font> deriveFont(int derivedStyle) const override {
		if (!fileUri.empty()) {
			return std::make_shared<FontWrapper>(family, fileUri, derivedStyle, size, face);
		}
		return tex::Font::_create(winrt::to_string(winrt::hstring(family)), derivedStyle, size);
	}

	bool operator==(tex::Font const& other) const override {
		auto const* backend = dynamic_cast<FontWrapper const*>(&other);
		return backend && size == backend->size && style == backend->style && face == backend->face;
	}

	bool operator!=(tex::Font const& other) const override {
		return !(*this == other);
	}

	canvas::Text::CanvasTextFormat text_format() const {
		auto format = canvas::Text::CanvasTextFormat();
		format.FontFamily(fileUri.empty() ? winrt::hstring(family) : fileUri + L"#" + family);
		format.FontSize(size);
		format.FontWeight(style & tex::BOLD ? winrt::Windows::UI::Text::FontWeights::Bold() : winrt::Windows::UI::Text::FontWeights::Normal());
		format.FontStyle(style & tex::ITALIC ? winrt::Windows::UI::Text::FontStyle::Italic : winrt::Windows::UI::Text::FontStyle::Normal);
		format.FontStretch(winrt::Windows::UI::Text::FontStretch::Normal);
		format.WordWrapping(canvas::Text::CanvasWordWrapping::NoWrap);
		return format;
	}

	canvas::Text::CanvasFontFace const& font_face() const { return face; }
};

class Win2DGraphics final : public tex::Graphics2D {
	canvas::CanvasDrawingSession session;
	tex::color color = tex::black;
	tex::Stroke stroke;
	FontWrapper const* font{};
	float3x2 transform{ 1, 0, 0, 1, 0, 0 };
	float scaleX = 1;
	float scaleY = 1;

	static winrt::Windows::UI::Color to_color(tex::color value) {
		return { static_cast<std::uint8_t>(tex::color_a(value)), static_cast<std::uint8_t>(tex::color_r(value)),
				 static_cast<std::uint8_t>(tex::color_g(value)), static_cast<std::uint8_t>(tex::color_b(value)) };
	}

	canvas::Geometry::CanvasStrokeStyle stroke_style() const {
		using namespace canvas::Geometry;
		auto value = CanvasStrokeStyle();
		value.StartCap(stroke.cap == tex::CAP_BUTT ? CanvasCapStyle::Flat : stroke.cap == tex::CAP_SQUARE ? CanvasCapStyle::Square
																										  : CanvasCapStyle::Round);
		value.EndCap(value.StartCap());
		value.LineJoin(stroke.join == tex::JOIN_BEVEL ? CanvasLineJoin::Bevel : stroke.join == tex::JOIN_MITER ? CanvasLineJoin::Miter
																											   : CanvasLineJoin::Round);
		if (stroke.miterLimit > 0) {
			value.MiterLimit(stroke.miterLimit);
		}
		return value;
	}

	void update_transform() {
		session.Transform(transform);
	}

public:
	explicit Win2DGraphics(canvas::CanvasDrawingSession const& drawingSession) : session(drawingSession) {
		session.TextAntialiasing(canvas::Text::CanvasTextAntialiasing::Grayscale);
	}

	void setColor(tex::color value) override { color = value; }
	tex::color getColor() const override { return color; }
	void setStroke(tex::Stroke const& value) override { stroke = value; }
	tex::Stroke const& getStroke() const override { return stroke; }
	void setStrokeWidth(float value) override { stroke.lineWidth = value; }
	tex::Font const* getFont() const override { return font; }
	void setFont(tex::Font const* value) override {
		auto const* backend = dynamic_cast<FontWrapper const*>(value);
		if (!backend) {
			throw winrt::hresult_invalid_argument(L"字体类型不受支持。");
		}
		font = backend;
	}

	void translate(float x, float y) override {
		transform = float3x2{ 1, 0, 0, 1, x, y } * transform;
		update_transform();
	}

	void scale(float x, float y) override {
		transform = float3x2{ x, 0, 0, y, 0, 0 } * transform;
		scaleX *= x;
		scaleY *= y;
		update_transform();
	}

	void rotate(float angle) override { rotate(angle, 0, 0); }

	void rotate(float angle, float x, float y) override {
		auto const sine = std::sin(angle);
		auto const cosine = std::cos(angle);
		transform = float3x2{
			cosine, sine, -sine, cosine, x - x * cosine + y * sine, y - x * sine - y * cosine
		} * transform;
		update_transform();
	}

	void reset() override {
		transform = { 1, 0, 0, 1, 0, 0 };
		scaleX = scaleY = 1;
		update_transform();
	}

	float sx() const override { return scaleX; }
	float sy() const override { return scaleY; }

	void drawChar(wchar_t character, float x, float y) override;
	void drawText(std::wstring const& text, float x, float y) override;

	void drawLine(float x1, float y1, float x2, float y2) override {
		session.DrawLine(x1, y1, x2, y2, to_color(color), stroke.lineWidth, stroke_style());
	}

	void drawRect(float x, float y, float width, float height) override {
		session.DrawRectangle(x, y, width, height, to_color(color), stroke.lineWidth, stroke_style());
	}

	void fillRect(float x, float y, float width, float height) override {
		session.FillRectangle(x, y, width, height, to_color(color));
	}

	void drawRoundRect(float x, float y, float width, float height, float radiusX, float radiusY) override {
		session.DrawRoundedRectangle(x, y, width, height, radiusX, radiusY, to_color(color), stroke.lineWidth, stroke_style());
	}

	void fillRoundRect(float x, float y, float width, float height, float radiusX, float radiusY) override {
		session.FillRoundedRectangle(x, y, width, height, radiusX, radiusY, to_color(color));
	}

	void drawTextLayout(canvas::Text::CanvasTextLayout const& layout, float x, float y) {
		auto const lines = layout.LineMetrics();
		if (lines.empty()) {
			return;
		}
		session.DrawTextLayout(layout, x, y - lines[0].Baseline, to_color(color));
	}
};

class Win2DTextLayout final : public tex::TextLayout {
	canvas::Text::CanvasTextLayout layout;

public:
	Win2DTextLayout(std::wstring const& source, FontWrapper const& font)
		: layout(canvas::CanvasDevice::GetSharedDevice(), source, font.text_format(),
				 std::numeric_limits<float>::max(), std::numeric_limits<float>::max()) {}

	void getBounds(tex::Rect& bounds) override {
		auto const lines = layout.LineMetrics();
		if (lines.empty()) {
			bounds = tex::Rect();
			return;
		}
		auto const metrics = layout.LayoutBoundsIncludingTrailingWhitespace();
		bounds = tex::Rect(metrics.X, -lines[0].Baseline, metrics.Width, lines[0].Height);
	}

	void draw(tex::Graphics2D& graphics, float x, float y) override {
		try {
			auto& win2d = dynamic_cast<Win2DGraphics&>(graphics);
			win2d.drawTextLayout(layout, x, y);
		}
		catch (const std::bad_cast&) {
			throw winrt::hresult_invalid_argument(L"Need Win2D Graphics Backend");
		}
	}
};

void Win2DGraphics::drawChar(wchar_t character, float x, float y) {
	std::uint32_t const codepoint = character;
	auto const indices = font->font_face().GetGlyphIndices({ &codepoint, 1 });
	if (indices.empty()) {
		return;
	}
	auto const metrics = font->font_face().GetGdiCompatibleGlyphMetrics(
		font->getSize(), 96.0f, { 1, 0, 0, 1, 0, 0 }, false, { indices.data(), indices.size() }, false);
	if (metrics.empty()) {
		return;
	}
	canvas::Text::CanvasGlyph glyph{ indices[0], metrics[0].AdvanceWidth, 0, 0 };
	auto brush = canvas::Brushes::CanvasSolidColorBrush(session, to_color(color));
	session.DrawGlyphRun({ x, y }, font->font_face(), font->getSize(), { &glyph, 1 }, false, 0, brush);
}

void Win2DGraphics::drawText(std::wstring const& text, float x, float y) {
	Win2DTextLayout(text, *font).draw(*this, x, y);
}

void initialize_latex() {
	auto& ctx = getMicroTeXContext();
	if (!ctx.latexInitialized) {
		tex::LaTeX::init(ctx.resourceRoot.string());
		ctx.latexInitialized = true;
	}
}

} // namespace

namespace tex {

using winrt::Windows::UI::Text::FontStretch;
using winrt::Windows::UI::Text::FontStyle;
using winrt::Windows::UI::Text::FontWeights;

Font* Font::create(std::string const& file, float size) {
	auto const path = std::filesystem::absolute(std::filesystem::path(winrt::to_hstring(file).c_str()));
	auto const uri = winrt::Windows::Foundation::Uri(L"file:///" + path.generic_wstring());
	auto const fonts = canvas::Text::CanvasFontSet(uri).Fonts();
	if (fonts.Size() == 0) {
		throw winrt::hresult_invalid_argument(L"无法读取字体文件。");
	}
	auto const face = fonts.GetAt(0);
	return new FontWrapper(getFontFaceFamilyName(face), uri.AbsoluteUri(), PLAIN, size, face);
}

sptr<Font> Font::_create(std::string const& name, int style, float size) {
	auto const family = winrt::to_hstring(name == "Serif" ? "Times New Roman" : name == "SansSerif" ? "Arial"
																									: name);
	auto const fontStyle = style & ITALIC ? FontStyle::Italic : FontStyle::Normal;
	auto const weight = style & BOLD ? FontWeights::Bold() : FontWeights::Normal();
	auto const fonts = canvas::Text::CanvasFontSet::GetSystemFontSet().GetMatchingFonts(family, weight, FontStretch::Normal, fontStyle).Fonts();
	if (fonts.Size() == 0) {
		throw winrt::hresult_invalid_argument(L"找不到指定的字体。");
	}
	return std::make_shared<FontWrapper>(std::wstring(family), L"", style, size, fonts.GetAt(0));
}

sptr<TextLayout> TextLayout::create(std::wstring const& src, sptr<Font> const& font) {
	auto const* backendFont = dynamic_cast<FontWrapper const*>(font.get());
	if (!backendFont) {
		throw winrt::hresult_invalid_argument(L"字体类型不受支持。");
	}
	return std::make_shared<Win2DTextLayout>(src, *backendFont);
}

} // namespace tex

namespace winrt::markdown::implementation {

winrt::Microsoft::UI::Xaml::Controls::Image TryRenderFormula(char const* formula, unsigned length, float dpi) {
	try {
		static auto& runtime = getMicroTeXContext();
		initialize_latex();
		auto const source = std::wstring(winrt::to_hstring(std::string_view(formula, length)));
		auto const foreground = winrt::Windows::UI::ViewManagement::UISettings().GetColorValue(
			winrt::Windows::UI::ViewManagement::UIColorType::Foreground);
		auto const color = tex::argb(foreground.A, foreground.R, foreground.G, foreground.B);
		auto render = std::unique_ptr<tex::TeXRender>(tex::LaTeX::parse(source, 720, 20.0f, 20.0f / 3, color));
		constexpr auto padding = 1.0f;
		auto const width = static_cast<float>(render->getWidth()) + padding * 2;
		auto const height = static_cast<float>(render->getHeight()) + padding * 2;
		auto device = canvas::CanvasDevice::GetSharedDevice();
		auto imageSource = canvas::UI::Xaml::CanvasImageSource(
			device, width, height, dpi, canvas::CanvasAlphaMode::Premultiplied);
		auto session = imageSource.CreateDrawingSession({ 0, 0, 0, 0 });
		Win2DGraphics graphics(session);
		render->draw(graphics, static_cast<int>(padding), static_cast<int>(padding));
		session.Close();

		auto image = winrt::Microsoft::UI::Xaml::Controls::Image();
		image.Source(imageSource);
		image.Width(width);
		image.Height(height);
		image.VerticalAlignment(winrt::Microsoft::UI::Xaml::VerticalAlignment::Center);
		return image;
	}
	catch (...) {
	}
	return nullptr;
}

} // namespace winrt::markdown::implementation
