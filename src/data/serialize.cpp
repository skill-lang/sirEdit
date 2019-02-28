#include <File.h>
#include <TypesOfType.h>
#include <sirEdit/data/serialize.hpp>
#include <sirEdit/utils/conver.hpp>
#include <unordered_map>
#include <tuple>
#include <functional>

using namespace sir::api;
using namespace sirEdit::data;
using namespace std;

//
// Serializer (read/write)
//

class RawData {
	public:
		unique_ptr<SkillFile> skillFile;

		vector<unique_ptr<Type>> allTypes;
		vector<Type*> baseTypes;

		RawData(SkillFile* sf) : skillFile(sf) {
			// Load Typs
			{
				// Phase 1: gen Types
				this->allTypes.resize(sf->UserdefinedType->size());
				size_t counter = 0;
				unordered_map<sir::UserdefinedType*, size_t> types;
				for(sir::UserdefinedType& i : sf->UserdefinedType->all()) {
					this->allTypes[counter] = move(sirEdit::utils::genBaseType(&i));
					this->allTypes[counter]->getID() = counter;
					types[&i] = counter;
					counter++;
				}

				// Phase 2: reference types
				{
					// Prepare
					unordered_map<sir::UserdefinedType*, Type*> lookupTypes;
					for(auto i : types)
						lookupTypes[i.first] = this->allTypes[i.second].get();

					// Add interfaces
					for(auto& i : sf->InterfaceType->all())
						if(i.getSuper() != nullptr)
							lookupTypes[i.getSuper()]->getSubTypes().push_back(lookupTypes[&i]);

					// Add classes
					for(auto &i : sf->ClassType->all())
						if(i.getSuper() != nullptr)
							lookupTypes[i.getSuper()]->getSubTypes().push_back(lookupTypes[&i]);
				}

				// Pahse 3: search unreferenced
				for(auto& i : this->allTypes) {
					bool good = true;
					for(auto& j : this->allTypes) {
						for(auto& k : j->getSubTypes())
							if(k == i.get()) {
								good = false;
								break;
							}
						if(!good)
							break;
					}
					if(good)
						this->baseTypes.push_back(i.get());
				}
			}
		}
};

extern shared_ptr<Serializer> sirEdit::data::Serializer::openFile(const string& file) {
	SkillFile* sf = SkillFile::read(file);
	Serializer* result = new Serializer(std::move(std::static_pointer_cast<void>(std::make_shared<RawData>(sf))));
	return move(shared_ptr<Serializer>(result));
}

//
// View access
//
struct ViewData {
	shared_ptr<RawData> raw;

	vector<Tool> tools;
};

sirEdit::data::View::View(shared_ptr<void> raw) {
	this->__raw = move(static_pointer_cast<void>(make_shared<ViewData>((ViewData) {move(static_pointer_cast<RawData>(raw)), {}})));
}

extern const vector<unique_ptr<Type>>& sirEdit::data::View::getTypes() const {
	return static_pointer_cast<ViewData>(this->__raw)->raw->allTypes;
}
extern const vector<Type*>& sirEdit::data::View::getBaseTypes() const {
	return static_pointer_cast<ViewData>(this->__raw)->raw->baseTypes;
}
extern const vector<Tool>& sirEdit::data::View::getTools() const {
	return static_pointer_cast<ViewData>(this->__raw)->tools;
}

extern View sirEdit::data::View::addTool(Tool tool) const {
	auto viewData = static_pointer_cast<ViewData>(this->__raw);
	View result = {viewData->raw};
	auto newViewData = static_pointer_cast<ViewData>(result.__raw);

	// New tools list
	{
		newViewData->tools.resize(viewData->tools.size() + 1);
		size_t i = 0;
		for(auto& j : viewData->tools) {
			newViewData->tools[i] = j;
			i++;
		}
		newViewData->tools[viewData->tools.size()] = tool;
	}

	return move(result);
}
