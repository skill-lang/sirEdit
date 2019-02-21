#include <File.h>
#include <TypesOfType.h>
#include <sirEdit/data/serialize.hpp>
#include <sirEdit/utils/conver.hpp>
#include <unordered_map>
#include <tuple>

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
					types[&i] = counter;
					counter++;
				}

				// Phase 2: reference types
				{
					// Prepare
					unordered_map<sir::UserdefinedType*, Type*> lookupTypes;
					for(auto i : types)
						lookupTypes[i.first] = this->allTypes[i.second].get();

					// Add references
					for(auto i : types)
						sirEdit::utils::updateBaseType(this->allTypes[i.second].get(), i.first, lookupTypes, this->baseTypes);
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

extern const vector<unique_ptr<Type>>& sirEdit::data::View::getTypes() const {
	return static_pointer_cast<RawData>(this->__raw)->allTypes;
}
extern const vector<Type*>& sirEdit::data::View::getBaseTypes() const {
	return static_pointer_cast<RawData>(this->__raw)->baseTypes;
}
