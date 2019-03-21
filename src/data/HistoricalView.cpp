#include <sirEdit/data/serialize.hpp>
#include <list>
#include <functional>

#include <iostream>

using namespace std;
using namespace sirEdit::data;

struct HistoricalData {
	size_t counter = 0;
};

sirEdit::data::HistoricalView::HistoricalView(View view) : staticView(move(view)) {
	this->data = static_pointer_cast<void>(make_shared<HistoricalData>());
}
sirEdit::data::HistoricalView::~HistoricalView() {
	// TODO: Check that thread stopped and all is safed
}

void sirEdit::data::HistoricalView::addTool(Tool tool) {
	this->staticView = this->staticView.addTool(tool);
}
void sirEdit::data::HistoricalView::setFieldStatus(const Tool& tool, Type& type, Field& field, FIELD_STATE state, const std::function<void(const Type&, const Field&, FIELD_STATE, FIELD_STATE)>& callback_field, const std::function<void(const Type&, FIELD_STATE, FIELD_STATE)>& callback_type) {
	// Set state
	if(state == FIELD_STATE::NO) {
		// TODO: Delete data
	}
	cout << "FIELD: " << field.getName() << endl;
	if(field.getStates().find(const_cast<Tool*>(&tool)) == field.getStates().end()) {
		cout << "LEN: " << field.getStates().size() << endl;
		field.getStates()[const_cast<Tool*>(&tool)] = move(unordered_map<Type*, FIELD_STATE>());
		cout << "LEN2: " << field.getStates().size() << endl;
	}
	cout << "LEN3: " << field.getStates().size() << endl;
	auto& tmp = static_cast<unordered_map<Type*, FIELD_STATE>&>(field.getStates()[const_cast<Tool*>(&tool)]);
	tmp[&type] = state;

	// TODO: Note by type

	// Callbacks
	FIELD_STATE abs_state = field.getToolType(tool);
	callback_field(type, field, abs_state, state);
	// TODO: Type callback
}
