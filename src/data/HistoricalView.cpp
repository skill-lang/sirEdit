#include <sirEdit/data/serialize.hpp>
#include <list>
#include <functional>

using namespace std;
using namespace sirEdit::data;


struct HistoricalData {
	size_t counter = 0;
};

sirEdit::data::HistoricalView::HistoricalView(View view) : staticView(move(view)) {
	this->data = static_pointer_cast<void>(make_shared<HistoricalData>());
}

void sirEdit::data::HistoricalView::addTool(Tool tool) {
	this->staticView = this->staticView.addTool(tool);
}
