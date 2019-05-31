#include <File.h>
#include <TypesOfType.h>
#include <sirEdit/main.hpp>
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

template<class SOURCE>
inline std::string _getName(SOURCE* type) {
	std::string result;
	for(int i = 0; i < type->getName()->getParts()->size(); i++)
		result += type->getName()->getParts()->get(i).string->c_str();
	return result;
}

template<class SOURCE>
inline std::unique_ptr<sirEdit::data::TypeWithFields> _loadFields(SOURCE* source) {
	if(source == nullptr)
		throw std::invalid_argument("Source is null pointer");
	std::vector<sirEdit::data::Field> fields;
	fields.resize(source->getFields()->size());
	size_t counter = 0;
	for(sir::FieldLike* i : *(source->getFields())) {
		fields[counter] = std::move(sirEdit::data::Field(_getName(i), "", "")); // TODO: Add comments and type
		counter++;
	}
	return std::move(std::make_unique<sirEdit::data::TypeWithFields>(_getName(source), "", fields)); // TODO: Add comments
}
inline sirEdit::data::Type* genBaseType(sir::UserdefinedType& uf) {
	std::string skillType = uf.skillName();
	sirEdit::data::Type* result;
	if(skillType == sir::ClassType::typeName) {
		std::unique_ptr<sirEdit::data::TypeWithFields> fields = std::move(_loadFields(static_cast<sir::ClassType*>(&uf)));
		result = new TypeClass(*(fields.get()), std::vector<sirEdit::data::TypeInterface*>(), nullptr);
	}
	else if(skillType == sir::InterfaceType::typeName) {
		std::unique_ptr<sirEdit::data::TypeWithFields> fields = std::move(_loadFields(static_cast<sir::InterfaceType*>(&uf)));
		result = new TypeInterface(*(fields.get()), std::vector<sirEdit::data::TypeInterface*>(), nullptr);
	}
	else
		throw std::invalid_argument(std::string("Unknown skill class type ") + skillType);
	return result;
}

namespace {
	class SerializerSIR : public Serializer
	{
		private:
			SkillFile* sf;
			unordered_map<Type*, sir::Type*> types;
			unordered_map<sir::Type*, Type*> typesInverse;
			unordered_map<Tool*, sir::Tool*> tools;

			template<class TYPE, class SOURCE>
			void addSuper(const SOURCE& source) {
				for(auto& i : source) {
					if(i.getSuper() == nullptr)
						static_cast<TYPE*>(typesInverse[&i])->getSuper() = nullptr;
					else {
						auto tmp = this->typesInverse.find(i.getSuper());
						if(tmp == this->typesInverse.end())
							throw; // TODO: Exception that should never happen
						static_cast<TYPE*>(typesInverse[&i])->getSuper() = tmp->second;
					}
				}
			}

		public:
			SerializerSIR(std::string path) {
				// Open file
				this->sf = SkillFile::open(path);
				if(this->sf == nullptr)
					throw; // TODO: Exception

				// Read types
				for(auto& i : this->sf->UserdefinedType->all()) {
					Type* tmpType = genBaseType(i);
					this->types[tmpType] = &i;
					this->typesInverse[&i] = tmpType;
				}

				// Gen super
				this->addSuper<TypeClass>(this->sf->ClassType->all());
				this->addSuper<TypeInterface>(this->sf->InterfaceType->all());

				// Read tools
				for(auto& i : this->sf->Tool->all()) {
					Tool* tmpTool = new Tool();
					tmpTool->getName() = std::move(std::string(i.getName()->begin(), i.getName()->end()));
					tmpTool->getDescription() = std::move(std::string(i.getDescription()->begin(), i.getDescription()->end()));
					tmpTool->getCommand() = std::move(std::string(i.getCommand()->begin(), i.getCommand()->end()));
					//for(auto& j : i.getSelectedFields()) {
					//	j.getData();
					//	Type* selectedType = this->typesInverse[j];
					//}
					this->tools[tmpTool] = &i;
				}

				// Run general updates
				this->updateRelationships();
			}
			~SerializerSIR() {
				delete this->sf;
			}

			void addBaseType(Type* type) {
				throw; // TODO:
			}
			void addBaseTool(Tool* tool) {
				auto tmp = this->sf->Tool->add();
				this->tools[tool] = tmp;
			}
			void getBaseTypes(std::function<void(Type*)> callbackFunc) {
				for(auto& i : this->types)
					callbackFunc(i.first);
			}
			void getBaseTools(std::function<void(Tool*)> callbackFunc) {
				for(auto& i : this->tools)
					callbackFunc(i.first);
			}
			void prepareSave() {
			}
			void save() {
				throw; // TODO:
			}
	};
}

namespace sirEdit::data {
	std::unique_ptr<Serializer> getSir(std::string path) {
		return std::move(std::make_unique<SerializerSIR>(path));
	}
}
