#include "BoardViewModel.h"
#include "pch.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <winrt/Windows.Data.Json.h>

#if __has_include("BoardViewModel.g.cpp")
#include "BoardViewModel.g.cpp"
#endif
#if __has_include("CardData.g.cpp")
#include "CardData.g.cpp"
#endif
#if __has_include("ColumnData.g.cpp")
#include "ColumnData.g.cpp"
#endif

namespace winrt::kanban::implementation {
using namespace winrt::Windows::Data::Json;
namespace wfc = winrt::Windows::Foundation::Collections;

static std::filesystem::path DataFilePath() {
	std::wstring buffer(MAX_PATH, L'\0');
	DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	if (length == 0 || length >= buffer.size()) {
		throw std::runtime_error("无法解析可执行文件路径");
	}

	buffer.resize(length);
	return std::filesystem::path(buffer).parent_path() / L"kanban.json";
}

static winrt::hstring NewGuid() {
	auto guid = winrt::Windows::Foundation::GuidHelper::CreateNewGuid();
	WCHAR buf[40];
	StringFromGUID2(guid, buf, 40);
	return winrt::hstring(buf);
}

// ── Serialization helpers ──────────────────────────────────────────

static JsonObject SerializeCard(winrt::kanban::CardData const& card) {
	auto obj = JsonObject();
	obj.SetNamedValue(L"id", JsonValue::CreateStringValue(card.Id()));
	obj.SetNamedValue(L"title", JsonValue::CreateStringValue(card.Title()));
	obj.SetNamedValue(L"description", JsonValue::CreateStringValue(card.Description()));
	return obj;
}

static JsonObject SerializeColumn(winrt::kanban::ColumnData const& col) {
	auto obj = JsonObject();
	obj.SetNamedValue(L"id", JsonValue::CreateStringValue(col.Id()));
	obj.SetNamedValue(L"title", JsonValue::CreateStringValue(col.Title()));

	auto cardsArray = JsonArray();
	for (auto const& card : col.Cards()) {
		cardsArray.Append(SerializeCard(card));
	}
	obj.SetNamedValue(L"cards", cardsArray);
	return obj;
}

static winrt::kanban::CardData DeserializeCard(JsonObject const& obj) {
	auto id = obj.GetNamedString(L"id");
	auto title = obj.GetNamedString(L"title");
	auto description = obj.GetNamedString(L"description");
	return winrt::make<CardData>(id, title, description);
}

static winrt::kanban::ColumnData DeserializeColumn(JsonObject const& obj) {
	auto id = obj.GetNamedString(L"id");
	auto title = obj.GetNamedString(L"title");
	auto col = winrt::make<ColumnData>(id, title);

	auto cardsArray = obj.GetNamedArray(L"cards");
	for (uint32_t i = 0; i < cardsArray.Size(); i++) {
		auto cardObj = cardsArray.GetObjectAt(i);
		col.Cards().Append(DeserializeCard(cardObj));
	}
	return col;
}

// ── BoardViewModel implementation ──────────────────────────────────

BoardViewModel::BoardViewModel() {
	m_columns = winrt::single_threaded_observable_vector<winrt::kanban::ColumnData>();
}

void BoardViewModel::AddColumn(winrt::hstring const& title) {
	auto col = winrt::make<ColumnData>(NewGuid(), title);
	m_columns.Append(col);
	Save();
}

void BoardViewModel::RemoveColumn(winrt::hstring const& id) {
	for (uint32_t i = 0; i < m_columns.Size(); i++) {
		if (m_columns.GetAt(i).Id() == id) {
			m_columns.RemoveAt(i);
			break;
		}
	}
	Save();
}

void BoardViewModel::AddCard(winrt::hstring const& columnId) {
	for (auto const& col : m_columns) {
		if (col.Id() == columnId) {
			auto card = winrt::make<CardData>(NewGuid(), L"New Card", L"");
			col.Cards().Append(card);
			Save();
			return;
		}
	}
}

void BoardViewModel::RemoveCard(winrt::hstring const& columnId, winrt::hstring const& cardId) {
	for (auto const& col : m_columns) {
		if (col.Id() == columnId) {
			auto cards = col.Cards();
			for (uint32_t i = 0; i < cards.Size(); i++) {
				if (cards.GetAt(i).Id() == cardId) {
					cards.RemoveAt(i);
					Save();
					return;
				}
			}
		}
	}
}

void BoardViewModel::MoveCardTo(
	winrt::hstring const& cardId,
	winrt::hstring const& sourceColumnId,
	winrt::hstring const& targetColumnId,
	int32_t insertIndex) {
	if (sourceColumnId == targetColumnId) return;

	winrt::kanban::ColumnData sourceColumn{ nullptr };
	winrt::kanban::ColumnData targetColumn{ nullptr };
	for (auto const& col : m_columns) {
		if (col.Id() == sourceColumnId) {
			sourceColumn = col;
		}
		else if (col.Id() == targetColumnId) {
			targetColumn = col;
		}
	}

	if (!sourceColumn || !targetColumn) return;

	winrt::kanban::CardData cardToMove{ nullptr };

	// 从源列中移除卡片
	{
		auto cards = sourceColumn.Cards();
		for (uint32_t i = 0; i < cards.Size(); i++) {
			if (cards.GetAt(i).Id() == cardId) {
				cardToMove = cards.GetAt(i);
				cards.RemoveAt(i);
				break;
			}
		}
	}

	if (!cardToMove) return;

	// 按 drop 位置插入目标列
	auto targetCards = targetColumn.Cards();
	uint32_t clamped = (std::min)(static_cast<uint32_t>(insertIndex), targetCards.Size());
	targetCards.InsertAt(clamped, cardToMove);

	Save();
}

void BoardViewModel::Save() {
	try {
		auto rootArray = JsonArray();
		for (auto const& col : m_columns) {
			rootArray.Append(SerializeColumn(col));
		}

		auto rootObj = JsonObject();
		rootObj.SetNamedValue(L"columns", rootArray);

		auto json = winrt::to_string(rootObj.Stringify());
		std::ofstream file(DataFilePath(), std::ios::binary | std::ios::trunc);
		if (!file) {
			throw std::runtime_error("无法打开 kanban.json 写入流");
		}
		file.write(json.data(), static_cast<std::streamsize>(json.size()));
		if (!file) {
			throw std::runtime_error("无法写入 kanban.json");
		}
	}
	catch (winrt::hresult_error const&) {
		return;
	}
	catch (std::exception const&) {
		return;
	}
}

void BoardViewModel::Load() {
	try {
		std::ifstream file(DataFilePath(), std::ios::binary);
		if (!file) {
			return;
		}

		std::string json{
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>() };
		auto jsonStr = winrt::to_hstring(json);

		auto rootObj = JsonObject::Parse(jsonStr);
		auto columnsArray = rootObj.GetNamedArray(L"columns");

		// 替换构造函数中设置的默认数据为已保存数据
		m_columns.Clear();
		for (uint32_t i = 0; i < columnsArray.Size(); i++) {
			m_columns.Append(DeserializeColumn(columnsArray.GetObjectAt(i)));
		}
	}
	catch (winrt::hresult_error const&) {
		return;
	}
	catch (std::exception const&) {
		return;
	}
}
} // namespace winrt::kanban::implementation
