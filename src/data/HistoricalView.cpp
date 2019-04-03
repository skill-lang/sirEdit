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
void sirEdit::data::HistoricalView::setFieldStatus(const Tool& tool, const Type& type, const Field& field, FIELD_STATE state, const std::function<void(const Type&, const Field&, FIELD_STATE, FIELD_STATE)>& callback_field, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
	// Set state
	FIELD_STATE old = tool.getFieldSetState(field, type);
	const_cast<Tool&>(tool).setFieldState(type, field, state);

	// Callback field
	{
		callback_field(type, field, tool.getFieldTransitiveState(field), state);
	}

	// Callback type
	{
		const Type* tmp_type = &type;
		while(tmp_type != nullptr) {
			callback_type(*tmp_type, tool.getTypeTransitiveState(*tmp_type), tool.getTypeSetState(*tmp_type));
			tmp_type = getSuper(*tmp_type);
		}
	}
}

inline void updateSubtypes(const Tool& tool, const Type& type, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
	for(auto& i : type.getSubTypes()) {
		auto tmp = tool.getTypeTransitiveState(type);
		if(tmp >= TYPE_STATE::READ) {
			callback_type(type, tmp, tool.getTypeSetState(type));
			updateSubtypes(tool, *i, callback_type);
		}
	}
}

void sirEdit::data::HistoricalView::setTypeStatus(const Tool& tool, const Type& type, TYPE_STATE state, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
	// Set type state
	TYPE_STATE oldTrans = tool.getTypeTransitiveState(type);
	const_cast<Tool&>(tool).setTypeState(type, state);

	// Callback
	TYPE_STATE newTrans = tool.getTypeTransitiveState(type);
	callback_type(type, newTrans, state);
	{
		// Update super classes
		const Type* current = getSuper(type);
		while(current != nullptr) {
			callback_type(*current, tool.getTypeTransitiveState(*current), tool.getTypeSetState(*current));
			current = getSuper(*current);
		}
	}
	if(oldTrans == TYPE_STATE::DELETE || newTrans == TYPE_STATE::DELETE) {
		// Update sub classes
		updateSubtypes(tool, type, callback_type);
	}
}
