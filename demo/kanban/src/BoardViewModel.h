#pragma once

#include "BoardViewModel.g.h"
#include "CardData.g.h"
#include "ColumnData.g.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::kanban::implementation {
struct CardData : CardDataT<CardData> {
	CardData() = default;
	CardData(winrt::hstring const& id, winrt::hstring const& title, winrt::hstring const& description)
		: m_id(id), m_title(title), m_description(description) {}

	winrt::hstring Id() const { return m_id; }
	winrt::hstring Title() const { return m_title; }
	void Title(winrt::hstring const& value) { m_title = value; }
	winrt::hstring Description() const { return m_description; }
	void Description(winrt::hstring const& value) { m_description = value; }

private:
	winrt::hstring m_id;
	winrt::hstring m_title;
	winrt::hstring m_description;
};

struct ColumnData : ColumnDataT<ColumnData> {
	ColumnData() = default;
	ColumnData(winrt::hstring const& id, winrt::hstring const& title)
		: m_id(id), m_title(title) {
		m_cards = winrt::single_threaded_observable_vector<winrt::kanban::CardData>();
	}

	winrt::hstring Id() const { return m_id; }
	winrt::hstring Title() const { return m_title; }
	void Title(winrt::hstring const& value) { m_title = value; }
	winrt::Windows::Foundation::Collections::IObservableVector<winrt::kanban::CardData> Cards() const { return m_cards; }

private:
	winrt::hstring m_id;
	winrt::hstring m_title;
	winrt::Windows::Foundation::Collections::IObservableVector<winrt::kanban::CardData> m_cards{ nullptr };
};

struct BoardViewModel : BoardViewModelT<BoardViewModel> {
	BoardViewModel();

	winrt::Windows::Foundation::Collections::IObservableVector<winrt::kanban::ColumnData> Columns() const { return m_columns; }

	void AddColumn(winrt::hstring const& title);
	void RemoveColumn(winrt::hstring const& id);
	void AddCard(winrt::hstring const& columnId);
	void RemoveCard(winrt::hstring const& columnId, winrt::hstring const& cardId);
	void MoveCardTo(winrt::hstring const& cardId, winrt::hstring const& sourceColumnId, winrt::hstring const& targetColumnId, int32_t insertIndex);
	void Save();
	void Load();

private:

	winrt::Windows::Foundation::Collections::IObservableVector<winrt::kanban::ColumnData> m_columns{ nullptr };
};
} // namespace winrt::kanban::implementation

namespace winrt::kanban::factory_implementation {
struct CardData : CardDataT<CardData, implementation::CardData> {};
struct ColumnData : ColumnDataT<ColumnData, implementation::ColumnData> {};
struct BoardViewModel : BoardViewModelT<BoardViewModel, implementation::BoardViewModel> {};
} // namespace winrt::kanban::factory_implementation
