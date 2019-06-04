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
inline std::unique_ptr<sirEdit::data::TypeWithFields> _loadFields(SOURCE* source, unordered_map<sir::FieldLike*, Field*>& inverse) {
	if(source == nullptr)
		throw std::invalid_argument("Source is null pointer");
	std::vector<sirEdit::data::Field> fields;
	fields.resize(source->getFields()->size());
	size_t counter = 0;
	for(sir::FieldLike* i : *(source->getFields())) {
		fields[counter] = std::move(sirEdit::data::Field(_getName(i), "", "")); // TODO: Add comments and type
		inverse[i] = &(fields[counter]);
		counter++;
	}
	return std::move(std::make_unique<sirEdit::data::TypeWithFields>(_getName(source), "", fields)); // TODO: Add comments
}
inline sirEdit::data::Type* genBaseType(sir::UserdefinedType& uf, unordered_map<sir::FieldLike*, Field*>& inverse) {
	std::string skillType = uf.skillName();
	sirEdit::data::Type* result;
	if(skillType == sir::ClassType::typeName) {
		std::unique_ptr<sirEdit::data::TypeWithFields> fields = std::move(_loadFields(static_cast<sir::ClassType*>(&uf), inverse));
		result = new TypeClass(*(fields.get()), std::vector<sirEdit::data::TypeInterface*>(), nullptr);
	}
	else if(skillType == sir::InterfaceType::typeName) {
		std::unique_ptr<sirEdit::data::TypeWithFields> fields = std::move(_loadFields(static_cast<sir::InterfaceType*>(&uf), inverse));
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
			unordered_map<sir::FieldLike*, Field*> fieldInverse;

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
					Type* tmpType = genBaseType(i, this->fieldInverse);
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

					// Load type states
					for(auto& j : *(i.getSelectedUserTypes())) {
						// Find type
						Type* type = nullptr;
						{
							sir::UserdefinedType* tmp_type = j.first;
							auto tmp = this->typesInverse.find(tmp_type);
							if(tmp != this->typesInverse.end())
								type = tmp->second;
							else
								continue;
						}

						// Parse state
						TYPE_STATE state;
						if(*(j.second) == "u")
							state = TYPE_STATE::NO;
						else if(*(j.second) == "r")
							state = TYPE_STATE::READ;
						else if(*(j.second) == "w")
							state = TYPE_STATE::WRITE;
						else if(*(j.second) == "d")
							state = TYPE_STATE::DELETE;
						else
							continue;

						// Set data
						tmpTool->setTypeState(*type, state);
					}

					// Load field states
					for(auto& j : *(i.getSelectedFields())) {
						// Find type
						Type* type = nullptr;
						{
							sir::UserdefinedType* tmp_type = j.first;
							auto tmp = this->typesInverse.find(tmp_type);
							if(tmp != this->typesInverse.end())
								type = tmp->second;
							else
								continue;
						}

						// Do fields
						for(auto& k : *(j.second)) {
							// Find field
							Field* field = nullptr;
							{
								sir::FieldLike* tmp_field = k.first;
								auto tmp = this->fieldInverse.find(tmp_field);
								if(tmp != this->fieldInverse.end())
									field = tmp->second;
								else
									continue;
							}

							// Parse state
							FIELD_STATE state;
							if(*(k.second) == "u")
								state = FIELD_STATE::NO;
							else if(*(k.second) == "r")
								state = FIELD_STATE::READ;
							else if(*(k.second) == "w")
								state = FIELD_STATE::WRITE;
							else if(*(k.second) == "c")
								state = FIELD_STATE::CREATE;
							else
								continue;

							// Set data
							tmpTool->setFieldState(*type, *field, state);
						}
					}
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
				throw; // TODO:
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
