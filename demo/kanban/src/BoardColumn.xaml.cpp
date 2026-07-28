#include "pch.h"

#include "BoardColumn.xaml.h"

#include "BoardView.xaml.h"

#include <cwctype>
#include <string_view>
#include <winrt/Microsoft.UI.Input.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>

#if __has_include("BoardColumn.g.cpp")
#include "BoardColumn.g.cpp"
#endif

namespace winrt::kanban::implementation {
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Input;
using namespace winrt::Microsoft::UI::Xaml::Media;
namespace wadt = winrt::Windows::ApplicationModel::DataTransfer;
namespace ws = winrt::Windows::System;
using wadt::DataPackageOperation;

static constexpr auto InitialLostFocusSuppressDuration = std::chrono::milliseconds{ 200 };

static winrt::hstring Trim(winrt::hstring const& value) {
	std::wstring_view text{ value.c_str(), value.size() };
	std::size_t begin = 0;
	std::size_t end = text.size();

	while (begin < end && std::iswspace(text[begin]) != 0) {
		begin++;
	}
	while (end > begin && std::iswspace(text[end - 1]) != 0) {
		end--;
	}
	return winrt::hstring{ text.substr(begin, end - begin) };
}

static DependencyObject FindDescendantByName(
	DependencyObject const& root,
	winrt::hstring const& name) {
	if (!root) return nullptr;

	if (auto element = root.try_as<FrameworkElement>()) {
		if (element.Name() == name) {
			return root;
		}
	}

	auto const childCount = VisualTreeHelper::GetChildrenCount(root);
	for (int32_t i = 0; i < childCount; i++) {
		auto child = VisualTreeHelper::GetChild(root, i);
		if (auto match = FindDescendantByName(child, name)) {
			return match;
		}
	}
	return nullptr;
}

template <typename T>
static T FindNearestNamedElement(
	DependencyObject const& source,
	winrt::hstring const& name) {
	auto current = source;
	while (current) {
		auto descendant = FindDescendantByName(current, name);
		if (descendant) {
			if (auto match = descendant.try_as<T>()) {
				return match;
			}
		}
		current = VisualTreeHelper::GetParent(current);
	}
	return nullptr;
}

static winrt::kanban::CardData FindCardData(DependencyObject const& source) {
	auto current = source;
	while (current) {
		if (auto element = current.try_as<FrameworkElement>()) {
			if (auto card = element.DataContext().try_as<winrt::kanban::CardData>()) {
				return card;
			}
		}
		current = VisualTreeHelper::GetParent(current);
	}
	return nullptr;
}

static winrt::kanban::BoardView FindParentBoardView(DependencyObject const& source) {
	auto current = source;
	while (current) {
		auto parent = VisualTreeHelper::GetParent(current);
		if (!parent) break;

		if (auto boardView = parent.try_as<winrt::kanban::BoardView>()) {
			return boardView;
		}
		current = parent;
	}
	return nullptr;
}

static bool IsPointerInsideElement(
	FrameworkElement const& element,
	UIElement const& relativeTo,
	winrt::Windows::Foundation::Point const& point) {
	if (!element || element.Visibility() != Visibility::Visible) return false;

	auto origin = element.TransformToVisual(relativeTo).TransformPoint({ 0, 0 });
	auto const right = origin.X + element.ActualWidth();
	auto const bottom = origin.Y + element.ActualHeight();
	return point.X >= origin.X && point.X <= right && point.Y >= origin.Y && point.Y <= bottom;
}

BoardColumn::BoardColumn() {
	InitializeComponent();
	AddHandler(
		UIElement::PointerPressedEvent(),
		winrt::box_value(PointerEventHandler{ this, &BoardColumn::OnPointerPressedForColumnDrag }),
		true);
	AddHandler(
		UIElement::PointerReleasedEvent(),
		winrt::box_value(PointerEventHandler{ this, &BoardColumn::OnPointerReleasedForColumnDrag }),
		true);
}

void BoardColumn::OnLoaded(
	IInspectable const&, RoutedEventArgs const&) {
	if (m_viewModel) return;

	if (auto boardView = FindParentBoardView(*this)) {
		m_viewModel = boardView.ViewModel();
	}
}

void BoardColumn::ColumnData(winrt::kanban::ColumnData const& value) {
	m_columnData = value;
	CardListView().ItemsSource(m_columnData.Cards());
	UpdateHeader();
}

void BoardColumn::UpdateHeader() {
	if (!m_columnData) return;
	TitleText().Text(m_columnData.Title());
	CountText().Text(std::to_wstring(m_columnData.Cards().Size()).c_str());
}

void BoardColumn::OnDeleteColumn(
	IInspectable const&, RoutedEventArgs const&) {
	if (m_viewModel && m_columnData) {
		m_viewModel.RemoveColumn(m_columnData.Id());
	}
}

void BoardColumn::OnAddCard(
	IInspectable const&, RoutedEventArgs const&) {
	if (!m_viewModel || !m_columnData) return;

	auto cards = m_columnData.Cards();
	auto beforeCount = cards.Size();
	m_viewModel.AddCard(m_columnData.Id());

	auto afterCount = cards.Size();
	if (afterCount > beforeCount) {
		CardListView().ScrollIntoView(cards.GetAt(afterCount - 1));
		UpdateHeader();
	}
}

void BoardColumn::OnToggleCollapsed(
	IInspectable const&, RoutedEventArgs const&) {
	UpdateCollapsedState();
}

void BoardColumn::OnPointerPressedForColumnDrag(
	IInspectable const&, PointerRoutedEventArgs const& e) {
	auto point = e.GetCurrentPoint(*this);
	if (point.PointerDeviceType() == Microsoft::UI::Input::PointerDeviceType::Mouse && !point.Properties().IsLeftButtonPressed()) {
		CancelColumnHeaderDrag();
		return;
	}
	if (!IsColumnHeaderBlankPoint(e)) {
		CancelColumnHeaderDrag();
		return;
	}

	BeginColumnHeaderDrag();
}

void BoardColumn::OnPointerReleasedForColumnDrag(
	IInspectable const&, PointerRoutedEventArgs const&) {
	CancelColumnHeaderDrag();
}

void BoardColumn::OnCardPointerWheelChanged(
	IInspectable const&,
	PointerRoutedEventArgs const& e) {
	if ((e.KeyModifiers() & ws::VirtualKeyModifiers::Shift) != ws::VirtualKeyModifiers::Shift) return;
	e.Handled(true);
}

void BoardColumn::UpdateCollapsedState() {
	auto isChecked = CollapseButton().IsChecked();
	auto const isCollapsed = isChecked ? isChecked.Value() : false;
	auto const contentVisibility = isCollapsed ? Visibility::Collapsed : Visibility::Visible;

	CardListView().Visibility(contentVisibility);
	AddCardButton().Visibility(contentVisibility);
	CollapseButton().Content(winrt::box_value(isCollapsed ? L"\uE70D" : L"\uE70E"));
}

bool BoardColumn::IsColumnHeaderBlankPoint(PointerRoutedEventArgs const& e) {
	auto const point = e.GetCurrentPoint(ColumnHeader()).Position();
	auto const relativeTo = ColumnHeader().as<UIElement>();
	if (point.X < 0 || point.X > ColumnHeader().ActualWidth() || point.Y < 0 || point.Y > ColumnHeader().ActualHeight()) {
		return false;
	}

	return !IsPointerInsideElement(TitleEditor(), relativeTo, point)
		   && !IsPointerInsideElement(CountBadge(), relativeTo, point)
		   && !IsPointerInsideElement(CollapseButton(), relativeTo, point)
		   && !IsPointerInsideElement(DeleteButton(), relativeTo, point);
}

void BoardColumn::BeginColumnHeaderDrag() {
	if (IsEditing() || !m_columnData) return;

	if (auto boardView = FindParentBoardView(*this)) {
		winrt::get_self<implementation::BoardView>(boardView)->BeginColumnHeaderDrag(m_columnData.Id());
	}
}

void BoardColumn::CancelColumnHeaderDrag() {
	if (auto boardView = FindParentBoardView(*this)) {
		winrt::get_self<implementation::BoardView>(boardView)->CancelPendingColumnDrag();
	}
}

void BoardColumn::BeginColumnTitleEdit() {
	if (!m_columnData) return;
	if (m_activeField != EditField::None) {
		FinishActiveEdit(true);
	}

	m_activeField = EditField::ColumnTitle;
	m_activeEditor = TitleEditor();
	m_activeText = TitleText();
	m_originalText = m_columnData.Title();

	TitleEditor().Text(m_originalText);
	TitleText().Visibility(Visibility::Collapsed);
	TitleEditor().Visibility(Visibility::Visible);
	SetEditingDragState(true);
	SuppressInitialLostFocus();
	FocusActiveEditorWhenReady();
}

void BoardColumn::FinishActiveEdit(bool commit) {
	if (m_activeField == EditField::ColumnTitle) {
		FinishColumnTitleEdit(commit);
		return;
	}

	if (m_activeEditor) {
		FinishCardEdit(m_activeEditor, commit);
	}
}

void BoardColumn::FinishColumnTitleEdit(bool commit) {
	if (m_finishingEdit || m_activeField != EditField::ColumnTitle) return;

	m_finishingEdit = true;
	auto nextTitle = Trim(TitleEditor().Text());
	auto const changed = nextTitle != m_originalText;

	if (commit && !nextTitle.empty()) {
		TitleText().Text(nextTitle);
		if (changed && m_columnData) {
			m_columnData.Title(nextTitle);
			if (m_viewModel) {
				m_viewModel.Save();
			}
		}
	}
	else {
		TitleEditor().Text(m_originalText);
		TitleText().Text(m_originalText);
	}

	TitleEditor().Visibility(Visibility::Collapsed);
	TitleText().Visibility(Visibility::Visible);
	SetEditingDragState(false);
	ResetEditState();
	m_finishingEdit = false;
}

void BoardColumn::ResetEditState() {
	m_activeField = EditField::None;
	m_activeCard = nullptr;
	m_activeEditor = nullptr;
	m_activeText = nullptr;
	m_originalText = L"";
	m_initialLostFocusRefocusQueued = false;
	m_initialLostFocusSuppressUntil = {};
}

void BoardColumn::SuppressInitialLostFocus() {
	m_initialLostFocusRefocusQueued = false;
	m_initialLostFocusSuppressUntil =
		std::chrono::steady_clock::now() + InitialLostFocusSuppressDuration;
}

void BoardColumn::FocusActiveEditorWhenReady() {
	auto weakThis = get_weak();
	DispatcherQueue().TryEnqueue([weakThis]() {
		if (auto self = weakThis.get()) {
			if (self->m_activeEditor && !self->m_finishingEdit) {
				self->m_activeEditor.Focus(FocusState::Programmatic);
				self->m_activeEditor.SelectAll();
				self->m_initialLostFocusSuppressUntil =
					std::chrono::steady_clock::now() + InitialLostFocusSuppressDuration;
			}
		}
	});
}

void BoardColumn::GuardKeyboardCommitFocus(TextBox const& editor) {
	m_keyboardCommitEditor = editor;
}

void BoardColumn::ClearKeyboardCommitFocusGuardWhenReady() {
	auto weakThis = get_weak();
	DispatcherQueue().TryEnqueue([weakThis]() {
		if (auto self = weakThis.get()) {
			self->m_keyboardCommitEditor = nullptr;
		}
	});
}

bool BoardColumn::ShouldSuppressInitialLostFocus(TextBox const& editor) {
	auto const now = std::chrono::steady_clock::now();
	auto const editorMatches = m_activeEditor && editor && editor == m_activeEditor;
	auto const shouldSuppress =
		editorMatches && !m_finishingEdit && now < m_initialLostFocusSuppressUntil;

	if (shouldSuppress && !m_initialLostFocusRefocusQueued) {
		m_initialLostFocusRefocusQueued = true;
		FocusActiveEditorWhenReady();
	}
	return shouldSuppress;
}

bool BoardColumn::IsEditing() const {
	return m_activeField != EditField::None;
}

void BoardColumn::FinishEditIfPointerOutside(
	PointerRoutedEventArgs const& e,
	UIElement const& relativeTo) {
	if (!m_activeEditor || !relativeTo || m_finishingEdit) return;

	auto const pointerPosition = e.GetCurrentPoint(relativeTo).Position();
	auto const editorOrigin = m_activeEditor.TransformToVisual(relativeTo).TransformPoint({ 0, 0 });
	auto const editorRight = editorOrigin.X + m_activeEditor.ActualWidth();
	auto const editorBottom = editorOrigin.Y + m_activeEditor.ActualHeight();
	auto const insideEditor =
		pointerPosition.X >= editorOrigin.X && pointerPosition.X <= editorRight && pointerPosition.Y >= editorOrigin.Y && pointerPosition.Y <= editorBottom;

	if (!insideEditor) {
		FinishActiveEdit(true);
	}
}

void BoardColumn::BeginCardEdit(
	DependencyObject const& source,
	EditField field) {
	if (field != EditField::CardTitle && field != EditField::CardDescription) return;
	if (m_activeField != EditField::None) {
		FinishActiveEdit(true);
	}

	auto card = FindCardData(source);
	auto textName = field == EditField::CardTitle ? L"CardTitleText" : L"CardDescriptionText";
	auto editorName = field == EditField::CardTitle ? L"CardTitleEditor" : L"CardDescriptionEditor";
	auto text = FindNearestNamedElement<TextBlock>(source, textName);
	auto editor = FindNearestNamedElement<TextBox>(source, editorName);
	if (!card || !text || !editor) return;

	m_activeField = field;
	m_activeCard = card;
	m_activeEditor = editor;
	m_activeText = text;
	m_originalText = field == EditField::CardTitle ? card.Title() : card.Description();

	editor.Text(m_originalText);
	text.Visibility(Visibility::Collapsed);
	editor.Visibility(Visibility::Visible);
	SetEditingDragState(true);
	SuppressInitialLostFocus();
	FocusActiveEditorWhenReady();
}

void BoardColumn::FinishCardEdit(TextBox const& editor, bool commit) {
	if (m_finishingEdit || !m_activeEditor || editor != m_activeEditor) return;

	m_finishingEdit = true;
	auto nextText = editor.Text();
	if (m_activeField == EditField::CardTitle) {
		nextText = Trim(nextText);
	}
	auto const changed = nextText != m_originalText;
	auto const isValid = m_activeField != EditField::CardTitle || !nextText.empty();

	if (commit && isValid && m_activeCard) {
		m_activeText.Text(nextText);
		if (changed) {
			if (m_activeField == EditField::CardTitle) {
				m_activeCard.Title(nextText);
			}
			else if (m_activeField == EditField::CardDescription) {
				m_activeCard.Description(nextText);
			}
			if (m_viewModel) {
				m_viewModel.Save();
			}
		}
	}
	else {
		editor.Text(m_originalText);
		m_activeText.Text(m_originalText);
	}

	editor.Visibility(Visibility::Collapsed);
	m_activeText.Visibility(Visibility::Visible);
	SetEditingDragState(false);
	ResetEditState();
	m_finishingEdit = false;
}

void BoardColumn::SetEditingDragState(bool editing) {
	CardListView().CanDragItems(!editing);
	CardListView().CanReorderItems(!editing);
	CardListView().AllowDrop(!editing);
}

void BoardColumn::OnColumnTitleDoubleTapped(
	IInspectable const&, DoubleTappedRoutedEventArgs const& e) {
	BeginColumnTitleEdit();
	e.Handled(true);
}

void BoardColumn::OnColumnTitleKeyDown(
	IInspectable const& sender, KeyRoutedEventArgs const& e) {
	if (e.Key() == ws::VirtualKey::Enter) {
		e.Handled(true);
		GuardKeyboardCommitFocus(sender.as<TextBox>());
		FinishColumnTitleEdit(true);
		ClearKeyboardCommitFocusGuardWhenReady();
	}
	else if (e.Key() == ws::VirtualKey::Escape) {
		FinishColumnTitleEdit(false);
		e.Handled(true);
	}
}

void BoardColumn::OnEditorLosingFocus(
	IInspectable const& sender, LosingFocusEventArgs const& e) {
	auto editor = sender.as<TextBox>();
	if (!m_keyboardCommitEditor || editor != m_keyboardCommitEditor) return;

	if (e.TryCancel()) {
		e.Handled(true);
	}
}

void BoardColumn::OnColumnTitleLostFocus(
	IInspectable const& sender, RoutedEventArgs const&) {
	if (ShouldSuppressInitialLostFocus(sender.as<TextBox>())) return;
	FinishColumnTitleEdit(true);
}

void BoardColumn::OnCardTitleDoubleTapped(
	IInspectable const& sender, DoubleTappedRoutedEventArgs const& e) {
	BeginCardEdit(sender.as<DependencyObject>(), EditField::CardTitle);
	e.Handled(true);
}

void BoardColumn::OnCardTitleKeyDown(
	IInspectable const& sender, KeyRoutedEventArgs const& e) {
	if (e.Key() == ws::VirtualKey::Enter) {
		e.Handled(true);
		GuardKeyboardCommitFocus(sender.as<TextBox>());
		FinishCardEdit(sender.as<TextBox>(), true);
		ClearKeyboardCommitFocusGuardWhenReady();
	}
	else if (e.Key() == ws::VirtualKey::Escape) {
		FinishCardEdit(sender.as<TextBox>(), false);
		e.Handled(true);
	}
}

void BoardColumn::OnCardTitleLostFocus(
	IInspectable const& sender, RoutedEventArgs const&) {
	if (ShouldSuppressInitialLostFocus(sender.as<TextBox>())) return;
	FinishCardEdit(sender.as<TextBox>(), true);
}

void BoardColumn::OnCardDescriptionDoubleTapped(
	IInspectable const& sender, DoubleTappedRoutedEventArgs const& e) {
	BeginCardEdit(sender.as<DependencyObject>(), EditField::CardDescription);
	e.Handled(true);
}

void BoardColumn::OnCardDescriptionLostFocus(
	IInspectable const& sender, RoutedEventArgs const&) {
	if (ShouldSuppressInitialLostFocus(sender.as<TextBox>())) return;
	FinishCardEdit(sender.as<TextBox>(), true);
}

void BoardColumn::OnDeleteCard(
	IInspectable const& sender, RoutedEventArgs const&) {
	if (!m_viewModel || !m_columnData) return;

	auto button = sender.as<Button>();
	auto cardId = winrt::unbox_value<winrt::hstring>(button.Tag());
	m_viewModel.RemoveCard(m_columnData.Id(), cardId);
}

void BoardColumn::OnDragItemsStarting(
	IInspectable const&,
	Controls::DragItemsStartingEventArgs const& e) {
	if (e.Items().Size() == 0 || !m_columnData) return;

	auto card = e.Items().GetAt(0).try_as<winrt::kanban::CardData>();
	if (!card) return;

	// Store card ID and source column ID in the data package
	e.Data().Properties().Insert(L"cardId", winrt::box_value(card.Id()));
	e.Data().Properties().Insert(L"sourceColumnId", winrt::box_value(m_columnData.Id()));
	e.Data().RequestedOperation(DataPackageOperation::Move);

	if (auto boardView = FindParentBoardView(*this)) {
		winrt::get_self<implementation::BoardView>(boardView)->BeginCardDrag();
	}
}

void BoardColumn::OnDragItemsCompleted(
	IInspectable const&,
	Controls::DragItemsCompletedEventArgs const&) {
	if (auto boardView = FindParentBoardView(*this)) {
		winrt::get_self<implementation::BoardView>(boardView)->EndCardDrag();
	}
}

void BoardColumn::OnListDragOver(
	IInspectable const&, DragEventArgs const& e) {
	// Accept move operation for cards from other columns
	e.AcceptedOperation(DataPackageOperation::Move);
}

void BoardColumn::OnListDrop(
	IInspectable const&, DragEventArgs const& e) {
	if (!m_viewModel || !m_columnData) return;

	auto props = e.DataView().Properties();

	auto cardIdObj = props.TryLookup(L"cardId");
	auto srcColIdObj = props.TryLookup(L"sourceColumnId");

	if (!cardIdObj || !srcColIdObj) return;

	auto cardId = winrt::unbox_value<winrt::hstring>(cardIdObj);
	auto srcColId = winrt::unbox_value<winrt::hstring>(srcColIdObj);

	if (srcColId == m_columnData.Id()) {
		// 同列重排：CanReorderItems 已完成移动，只需持久化
		m_viewModel.Save();
		return;
	}

	// 根据 drop 位置计算插入索引
	auto dropPoint = e.GetPosition(CardListView());
	uint32_t insertIndex = m_columnData.Cards().Size();
	uint32_t itemCount = m_columnData.Cards().Size();
	for (uint32_t i = 0; i < itemCount; i++) {
		auto container = CardListView().ContainerFromIndex(i);
		if (!container) continue;

		auto elem = container.as<FrameworkElement>();
		auto origin = elem.TransformToVisual(CardListView()).TransformPoint({ 0, 0 });
		if (dropPoint.Y < origin.Y + elem.ActualHeight()) {
			insertIndex = i;
			break;
		}
	}

	m_viewModel.MoveCardTo(cardId, srcColId, m_columnData.Id(), insertIndex);
}
} // namespace winrt::kanban::implementation
