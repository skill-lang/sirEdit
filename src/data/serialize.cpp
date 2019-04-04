#include <File.h>
#include <TypesOfType.h>
#include <sirEdit/main.hpp>
#include <sirEdit/data/serialize.hpp>
#include <sirEdit/utils/conver.hpp>
#include <unordered_map>
#include <tuple>
#include <functional>

#include <iostream>

using namespace sir::api;
using namespace sirEdit::data;
using namespace std;

//
// Serializer (read/write)
//

inline void checkSuper(const Type* type) {
	auto tmp = getSuper(*type);
	if(tmp != nullptr)
		checkSuper(tmp);
}

class RawData {
	public:
		unique_ptr<SkillFile> skillFile;
		std::string file;
		vector<shared_ptr<Type>> allTypes;

		vector<Type*> allTypesAccess;
		vector<Type*> baseTypes;

		RawData(SkillFile* sf, std::string file) : skillFile(sf), file(move(file)) {
			// Load Typs
			{
				// Phase 1: gen Types
				this->allTypes.resize(sf->UserdefinedType->size());
				this->allTypesAccess.resize(sf->UserdefinedType->size());
				size_t counter = 0;
				unordered_map<sir::UserdefinedType*, size_t> types;
				for(sir::UserdefinedType& i : sf->UserdefinedType->all()) {
					this->allTypes[counter] = move(sirEdit::utils::genBaseType(&i));
					this->allTypes[counter]->getID() = counter;
					this->allTypesAccess[counter] = this->allTypes[counter].get();
					cout << "Address " << getSuper(*this->allTypesAccess[counter]) << endl;
					types[&i] = counter;
					counter++;
				}


				for(auto& i: this->allTypes)
					checkSuper(i.get());

				// Phase 2: reference types
				{
					// Prepare
					unordered_map<sir::UserdefinedType*, Type*> lookupTypes;
					for(auto i : types)
						lookupTypes[i.first] = this->allTypes[i.second].get();

					// Add interfaces
					for(auto& i : sf->InterfaceType->all())
						if(i.getSuper() != nullptr) {
							lookupTypes[i.getSuper()]->getSubTypes().push_back(lookupTypes[&i]);
							dynamic_cast<TypeInterface*>(lookupTypes[&i])->getSuper() = lookupTypes[i.getSuper()];
						}
						else
							dynamic_cast<TypeInterface*>(lookupTypes[&i])->getSuper() = nullptr;

					// Add classes
					for(auto &i : sf->ClassType->all())
						if(i.getSuper() != nullptr) {
							lookupTypes[i.getSuper()]->getSubTypes().push_back(lookupTypes[&i]);
							dynamic_cast<TypeClass*>(lookupTypes[&i])->getSuper() = lookupTypes[i.getSuper()];
						}
						else
							dynamic_cast<TypeClass*>(lookupTypes[&i])->getSuper() = nullptr;
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

				// Phase 4: Check super
				for(auto& i: this->allTypes)
					checkSuper(i.get());
			}
		}
};

extern shared_ptr<Serializer> sirEdit::data::Serializer::openFile(const string& file) {
	SkillFile* sf = SkillFile::open(file);
	Serializer* result = new Serializer(std::move(std::static_pointer_cast<void>(std::make_shared<RawData>(sf, file))));
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

extern const vector<Type*>& sirEdit::data::View::getTypes() const {
	return static_pointer_cast<ViewData>(this->__raw)->raw->allTypesAccess;
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

extern bool sirEdit::data::View::saveToFile() {
	ViewData* viewData = static_pointer_cast<ViewData>(this->__raw).get();

	// Remoe old tools
	for(auto& i: *(viewData->raw->skillFile->Tool)) {
		// TODO: delete
	}

	// Add tools
	for(auto& i : viewData->tools) {
		sir::Tool* tool = viewData->raw->skillFile->Tool->add();
		{
			auto tmp = viewData->raw->skillFile->strings->add(i.getName().c_str());
			tool->setName(tmp);
		}
	}

	// Save
	viewData->raw->skillFile->flush();
	return true;
}
