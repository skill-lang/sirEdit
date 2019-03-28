#include <sirEdit/data/serialize.hpp>
#include <list>
#include <functional>

#include <sirEdit/main.hpp>

using namespace std;
using namespace sirEdit;
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
void sirEdit::data::HistoricalView::setFieldStatus(const Tool& tool, Type& type, Field& field, FIELD_STATE state, const std::function<void(const Type&, const Field&, FIELD_STATE, FIELD_STATE)>& callback_field, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
	// Set state
	FIELD_STATE old = field.getToolSet(tool, type);
	{
		auto tmp_tool = field.getStates().find(const_cast<Tool*>(&tool));
		if(state == FIELD_STATE::NO) {
			if(tmp_tool != field.getStates().end()) {
				auto tmp_entry = tmp_tool->second.find(&type);
				if(tmp_entry != tmp_tool->second.end()) {
					// Remove and decrese set states
					tmp_tool->second.erase(tmp_entry);
					type.getCountFieldsSet()[const_cast<Tool*>(&tool)]--;
				}
			}
		}
		else
		{
			// Check to create tool
			if(tmp_tool == field.getStates().end()) {
				field.getStates()[const_cast<Tool*>(&tool)] = move(unordered_map<Type*, FIELD_STATE>());
				tmp_tool = field.getStates().find(const_cast<Tool*>(&tool));
			}

			// Set entry
			tmp_tool->second[&type] = state;

			// Update counter
			if(old <= FIELD_STATE::UNUSED & state >= FIELD_STATE::READ) {
				auto tmp = type.getCountFieldsSet().find(const_cast<Tool*>(&tool));
				if(tmp == type.getCountFieldsSet().end())
					type.getCountFieldsSet()[const_cast<Tool*>(&tool)] = 1;
				else
					tmp->second++;
			}
			else if(old >= FIELD_STATE::READ & state <= FIELD_STATE::UNUSED) {
				auto tmp = type.getCountFieldsSet().find(const_cast<Tool*>(&tool));
				if(tmp == type.getCountFieldsSet().end())
					throw; // This should NEVER happen!
				tmp->second--;
			}
		}
	}

	// Callback field
	{
		FIELD_STATE abs_state = field.getToolType(tool);
		callback_field(type, field, abs_state, state);
	}

	// Callback type
	{
		const Type* tmp_type = &type;
		while(tmp_type != nullptr) {
			callback_type(*tmp_type, this->getStaticView().getTypeTransitive(tool, *tmp_type), tmp_type->getToolSet(tool));
			tmp_type = getSuper(*tmp_type);
		}
	}
}

inline void updateSubtypes(const Tool& tool, const Type& type, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
	for(auto& i : type.getSubTypes()) {
		auto tmp = views->getStaticView().getTypeTransitive(tool, type);
		if(tmp >= TYPE_STATE::READ) {
			callback_type(type, tmp, type.getToolSet(tool));
			updateSubtypes(tool, *i, callback_type);
		}
	}
}

void sirEdit::data::HistoricalView::setTypeStatus(const Tool& tool, Type& type, TYPE_STATE state, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
	// Set type state
	TYPE_STATE old = type.getToolSet(tool);
	type.getState()[const_cast<Tool*>(&tool)] = state;

	// Callback
	callback_type(type, this->getStaticView().getTypeTransitive(tool, type), type.getToolSet(tool));
	if(old == TYPE_STATE::DELETE || state == TYPE_STATE::DELETE) {
		updateSubtypes(tool, type, callback_type);
	}
	{
		// Update parents
		const Type* tmp = getSuper(type);
		while(tmp != nullptr) {
			callback_type(*tmp, this->getStaticView().getTypeTransitive(tool, *tmp), tmp->getToolSet(tool));
			tmp = getSuper(*tmp);
		}
	}
}
