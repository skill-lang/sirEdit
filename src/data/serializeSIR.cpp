#include <File.h>
#include <TypesOfType.h>
#include <sirEdit/main.hpp>
#include <sirEdit/data/serialize.hpp>
#include <sirEdit/utils/conver.hpp>
#include <unordered_map>
#include <tuple>
#include <functional>
#include <stdexcept>

using namespace sir::api;
using namespace sirEdit::data;
using namespace std;

//
// Serializer (read/write)
//

template<class SOURCE>
inline std::string parseString(SOURCE* source, string spliter = "") {
	std::string result;
	bool first = true;
	for(auto& i : *source) {
		if(first)
			first = false;
		else
			result += spliter;
		result += *i;
	}
	return result;
}
inline std::string parseComment(sir::Comment* comment) {
	if(comment == nullptr)
		return "";
	auto tmp = comment->getText();
	if(tmp == nullptr)
		return "";
	return parseString(tmp, " ");
	// TODO: Tags
}

template<class FUNC>
inline void findFieldInSIR(sir::Type* type, FUNC func) {
	std::string skillType = type->skillName();
	if(skillType == sir::ClassType::typeName) {
		func(static_cast<sir::ClassType*>(type)->getFields());
	}
	else if(skillType == sir::InterfaceType::typeName) {
		func(static_cast<sir::InterfaceType*>(type)->getFields());
	}
	else
		throw std::invalid_argument(std::string("Unknown skill class type ") + skillType);
}

template<class SOURCE>
inline std::unique_ptr<sirEdit::data::TypeWithFields> _loadFields(SOURCE* source) {
	if(source == nullptr)
		throw std::invalid_argument("Source is null pointer");
	std::vector<sirEdit::data::Field> fields;
	fields.resize(source->getFields()->size());
	size_t counter = 0;
	for(sir::FieldLike* i : *(source->getFields())) {
		fields[counter] = std::move(sirEdit::data::Field(parseString(i->getName()->getParts()), parseComment(i->getComment()), {}));
		counter++;
	}
	if constexpr(is_same<SOURCE, sir::EnumType>::value) {
		if(source->getInstances() != nullptr) {
			fields.resize(counter + source->getInstances()->size());
			for(auto& i : *(source->getInstances())) {
				fields[counter] = sirEdit::data::Field(parseString(i->getParts()), "", {});
				counter++;
			}
		}
	}
	return std::move(std::make_unique<sirEdit::data::TypeWithFields>(parseString(source->getName()->getParts()), parseComment(source->getComment()), std::move(fields))); // TODO: Add comments
}
inline sirEdit::data::Type* genBaseType(sir::UserdefinedType& uf) {
	std::string skillType = uf.skillName();
	sirEdit::data::Type* result;

	// Create type
	if(skillType == sir::ClassType::typeName) {
		std::unique_ptr<sirEdit::data::TypeWithFields> fields = std::move(_loadFields(static_cast<sir::ClassType*>(&uf)));
		result = new TypeClass(std::move(*(fields.get())), std::vector<sirEdit::data::TypeInterface*>(), nullptr);
	}
	else if(skillType == sir::InterfaceType::typeName) {
		std::unique_ptr<sirEdit::data::TypeWithFields> fields = std::move(_loadFields(static_cast<sir::InterfaceType*>(&uf)));
		result = new TypeInterface(std::move(*(fields.get())), std::vector<sirEdit::data::TypeInterface*>(), nullptr);
	}
	else if(skillType == sir::EnumType::typeName) {
		std::unique_ptr<sirEdit::data::TypeWithFields> fields = std::move(_loadFields(static_cast<sir::EnumType*>(&uf)));
		result = new TypeEnum(std::move(*(fields.get())), nullptr);
	}
	else if(skillType == sir::TypeDefinition::typeName) {
		result = new TypeTypedef(parseString(uf.getName()->getParts()), parseComment(uf.getComment()), nullptr);
	}
	//else
	//	throw std::invalid_argument(std::string("Unknown skill class type ") + skillType);

	// Hints
	if(uf.getHints() != nullptr)
		for(auto& i : *(uf.getHints())) {
			auto tmp = result->getHints().find(*(i->getName()));
			if(tmp == result->getHints().end())
				tmp = result->getHints().insert(make_pair(string(*(i->getName())), vector<string>())).first;
			for(auto& j : *(i->getArguments()))
				tmp->second.push_back(*j);

		}
	if(uf.getRestrictions() != nullptr)
		for(auto& i : *(uf.getRestrictions())) {
			auto tmp = result->getRestrictions().find(*(i->getName()));
			if(tmp == result->getRestrictions().end())
				tmp = result->getRestrictions().insert(make_pair(string(*(i->getName())), vector<string>())).first;
			for(auto& j : *(i->getArguments()))
				tmp->second.push_back(*j);

		}
	return result;
}

inline void updateFields(Type* type, sir::Type* sirType, unordered_map<Field*, sir::FieldLike*>& fields, unordered_map<sir::FieldLike*, Field*>& inverses) {
	// Find fields
	TypeWithFields* twf = dynamic_cast<TypeWithFields*>(type);
	if(twf == nullptr)
		return; // Typedef

	// Find fields in sir
	decltype(static_cast<sir::ClassType*>(nullptr)->getFields()) sirTWF;
	findFieldInSIR(sirType, [&sirTWF](auto tmp) -> void {
		sirTWF = tmp;
	});

	// Update field information
	auto j = sirTWF->begin();
	for(auto i = twf->getFields().begin(); i != twf->getFields().end(); i++, j++) {
		if(i->getName() != parseString((*j)->getName()->getParts()))
			throw; // That should never happen!
		fields[&(*i)] = *j;
		inverses[*j] = &(*i);
	}
}

inline Type* addBuildinType(sir::Type* type, unordered_map<sir::Type*, Type*>& typeInverse) {
	// Check type
	if(dynamic_cast<sir::UserdefinedType*>(type) != nullptr)
		throw; // That sould not happen!
	if(dynamic_cast<sir::SingleBaseTypeContainer*>(type) != nullptr)
		throw; // That sould not happen!

	// Add new type
	sir::SimpleType* tmp_st = dynamic_cast<sir::SimpleType*>(type);
	if(tmp_st == nullptr)
		throw; // That should not happen
	Type* tmp = new Type(parseString(tmp_st->getName()->getParts()), "");
	typeInverse[type] = tmp;
	return tmp;
}
inline void updateField(Field& field, unordered_map<Field*, sir::FieldLike*>& fields, unordered_map<sir::Type*, Type*>& typeInverse) {
	// Find field
	sir::FieldLike* orignalField = nullptr;
	{
		auto tmp = fields.find(&field);
		if(tmp == fields.end()) {
			std::string tmp = "";
			for(auto& i : fields)
				tmp += i.first->getName() +  ", ";
			throw runtime_error("Can't find field " + field.getName() + " : " + tmp);
		}
		else
			orignalField = tmp->second;
	}

	// TODO: Views and custome type

	// Update field
	sir::MapType* tmp_map;
	sir::ConstantLengthArrayType* tmp_carray;
	sir::SingleBaseTypeContainer* tmp_array;
	if(dynamic_cast<sir::UserdefinedType*>(orignalField->getType()) != nullptr || dynamic_cast<sir::SimpleType*>(orignalField->getType()) != nullptr) { // Simple type
		field.getType().combination = FieldType::TYPE_COMBINATION::SINGLE_TYPE;
		Type* tmp_type;
		{
			auto tmp = typeInverse.find(orignalField->getType());
			if(tmp == typeInverse.end())
				tmp_type = addBuildinType(orignalField->getType(), typeInverse);
			else
				tmp_type = tmp->second;
		}
		field.getType().types = {tmp_type};
	}
	else if((tmp_map = dynamic_cast<sir::MapType*>(orignalField->getType())) != nullptr) { // Map type
		field.getType().combination = FieldType::TYPE_COMBINATION::MAP;
		for(auto& i : *(tmp_map->getBase())) {
			Type* tmp_type;
			{
				auto tmp = typeInverse.find(i);
				if(tmp == typeInverse.end())
					tmp_type = addBuildinType(i, typeInverse);
				else
					tmp_type = tmp->second;
			}
			field.getType().types.push_back(tmp_type);
		}
	}
	else if((tmp_carray = dynamic_cast<sir::ConstantLengthArrayType*>(orignalField->getType())) != nullptr) { // Constant array
		if(*(tmp_carray->getKind()) != "array")
			throw; // Bad array
		field.getType().combination = FieldType::TYPE_COMBINATION::STATIC_ARRAY;
		Type* tmp_type;
		{
			auto tmp = typeInverse.find(tmp_carray->getBase());
			if(tmp == typeInverse.end())
				tmp_type = addBuildinType(tmp_carray->getBase(), typeInverse);
			else
				tmp_type = tmp->second;
		}
		field.getType().types = {tmp_type};
		field.getType().arraySize = tmp_carray->getLength();
	}
	else if((tmp_array = dynamic_cast<sir::SingleBaseTypeContainer*>(orignalField->getType())) != nullptr) { // Array types
		if(*(tmp_array->getKind()) == "array")
			field.getType().combination = FieldType::TYPE_COMBINATION::DYNAMIC_ARRAY;
		else if(*(tmp_array->getKind()) == "list")
			field.getType().combination = FieldType::TYPE_COMBINATION::LIST;
		else if(*(tmp_array->getKind()) == "set")
			field.getType().combination = FieldType::TYPE_COMBINATION::SET;
		else
			throw; // Unkown type

		// Set type
		Type* tmp_type;
		{
			auto tmp = typeInverse.find(tmp_array->getBase());
			if(tmp == typeInverse.end())
				tmp_type = addBuildinType(tmp_array->getBase(), typeInverse);
			else
				tmp_type = tmp->second;
		}
		field.getType().types = {tmp_type};
	}
	else {
		throw std::runtime_error(std::string("Unkown type ") + orignalField->getType()->skillName()); // Unkown type
	}
}

template<class TYPE1, class TYPE2>
inline void _addInterfacesHelper(TYPE1* type, unordered_map<Type*, sir::Type*>& types, unordered_map<sir::Type*, Type*>& typeInverse) {
	// Check
	if(type == nullptr)
		throw; // BAD CAST!!!

	// Find base type
	TYPE2* tmpSirType;
	{
		auto tmp = types.find(type);
		if(tmp == types.end())
			throw; // That should never happen!
		tmpSirType = dynamic_cast<TYPE2*>(tmp->second);
		if(tmpSirType == nullptr)
			throw; // That should never happen!
	}

	// Update interfaces
	if(tmpSirType->getInterfaces() != nullptr)
		for(auto& i : *(tmpSirType->getInterfaces())) {
			auto tmp = typeInverse.find(i);
			if(tmp == typeInverse.end())
				throw; // That should never happen!
			TypeInterface* tmp2 = dynamic_cast<TypeInterface*>(tmp->second);
			if(tmp2 == nullptr)
				throw; // That should never happen!
			type->getInterfaces().push_back(tmp2);
		}
}
inline void addInterfaces(Type& type, unordered_map<Type*, sir::Type*>& types, unordered_map<sir::Type*, Type*>& typeInverse) {
	doBaseType(type, []() -> void {}, [&type, &types, &typeInverse]() -> void {
		_addInterfacesHelper<TypeInterface, sir::InterfaceType>(dynamic_cast<TypeInterface*>(&type), types, typeInverse);
	}, [&type, &types, &typeInverse]() -> void {
		_addInterfacesHelper<TypeClass, sir::ClassType>(dynamic_cast<TypeClass*>(&type), types, typeInverse);
	}, []() -> void {}, []() -> void {});
}

namespace {
	class SerializerSIR : public Serializer
	{
		private:
			SkillFile* sf;
			unordered_map<Type*, sir::Type*> types;
			unordered_map<sir::Type*, Type*> typesInverse;
			unordered_map<Tool*, sir::Tool*> tools;
			unordered_map<Field*, sir::FieldLike*> field;
			unordered_map<sir::FieldLike*, Field*> fieldInverse;

			list<Tool*> toAdd;
			list<sir::Tool*> toRemove;
			list<Type*> addType;

			list<Type*> saveAddType;
			unordered_map<Tool*, sir::Tool*> saveTools;
			unordered_map<Tool*, Tool> saveToolData;

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

			auto newSirString(const std::string& source) {
				return this->sf->strings->add(source.c_str(), source.size());
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

				// Add interfaces
				for(auto& i : this->types)
					addInterfaces(*(i.first), this->types, this->typesInverse);

				// Update fields pass 1
				for(auto& i : this->types)
					updateFields(i.first, i.second, this->field, this->fieldInverse);

				// Update fileds pass 2
				for(auto& i : this->types) {
					TypeWithFields* tmp = dynamic_cast<TypeWithFields*>(i.first);
					if(tmp == nullptr)
						continue; // Alias type
					for(auto& j : tmp->getFields())
						updateField(j, this->field, this->typesInverse);
				}

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
			void removeBaseType(Type* type) {
				throw; // TODO:
			}
			void addBaseTool(Tool* tool) {
				this->toAdd.push_back(tool);
				//auto tmp = this->sf->Tool->add();
				this->tools[tool] = nullptr;
			}
			void removeBaseTool(Tool* tool) {
				this->toRemove.push_back(tools[tool]);
				//this->sf->free(this->tools[tool]);
				this->tools.erase(tool);
				delete tool;
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
				// New tools
				for(auto& i : this->toAdd)
					if(this->tools.count(i) != 0)
						this->tools[i] = this->sf->Tool->add();
				this->toAdd.clear();

				// Remove old tools
				for(auto& i : this->toRemove)
					if(i != nullptr)
						this->sf->free(i);
				this->toRemove.clear();

				// Copy Tool states
				this->saveTools = this->tools;
				for(auto& i : this->tools)
					this->saveToolData.insert({i.first, *(i.first)});
				// TODO: Types
			}
			void save() {
				// Copy tool data
				for(auto& i : this->saveToolData) {
					sir::Tool* sTool = this->saveTools[i.first];
					sTool->setName(this->newSirString(i.second.getName()));
					sTool->setCommand(this->newSirString(i.second.getCommand()));
					sTool->setDescription(this->newSirString(i.second.getDescription()));

					// Update types
					auto types = new ::skill::api::Map<::sir::UserdefinedType*, ::skill::api::String>();
					if(sTool->getSelectedUserTypes() !=  nullptr)
						delete sTool->getSelectedUserTypes();
					sTool->setSelectedUserTypes(types);
					for(auto& j : i.second.getStatesTypes()) {
						switch(get<1>(j.second)) {
							case TYPE_STATE::READ:
								(*types)[static_cast<sir::UserdefinedType*>(this->types[const_cast<Type*>(j.first)])] = this->newSirString("r");
								break;
							case TYPE_STATE::WRITE:
								(*types)[static_cast<sir::UserdefinedType*>(this->types[const_cast<Type*>(j.first)])] = this->newSirString("w");
								break;
							case TYPE_STATE::DELETE:
								(*types)[static_cast<sir::UserdefinedType*>(this->types[const_cast<Type*>(j.first)])] = this->newSirString("d");
								break;
							case TYPE_STATE::UNUSED:
								(*types)[static_cast<sir::UserdefinedType*>(this->types[const_cast<Type*>(j.first)])] = this->newSirString("u");
								break;
							case TYPE_STATE::NO:
								break;
						}
					}

					// Update fields
					{
						auto fields = new ::skill::api::Map<::sir::UserdefinedType*, ::skill::api::Map<::sir::FieldLike*, ::skill::api::String>*>();
						if(sTool->getSelectedFields() != nullptr)
							delete sTool->getSelectedFields();
						sTool->setSelectedFields(fields);

						// Copy field data
						for(auto& j : i.second.getStatesFields()) {
							for(auto& k : j.second) {
								// Get string
								bool found = false;
								::skill::api::String s;
								switch(k.second) {
									case FIELD_STATE::READ:
										s = this->newSirString("r");
										found = true;
										break;
									case FIELD_STATE::WRITE:
										s = this->newSirString("w");
										found = true;
										break;
									case FIELD_STATE::CREATE:
										s = this->newSirString("c");
										found = true;
										break;
									case FIELD_STATE::UNUSED:
										s = this->newSirString("u");
										found = true;
										break;
									case FIELD_STATE::NO:
										break;
								}
								if(!found)
									continue;

								// Insert data
								auto tmp = fields->find(static_cast<sir::UserdefinedType*>(this->types[const_cast<Type*>(k.first)]));
								if(tmp == fields->end()) {
									fields->insert({static_cast<sir::UserdefinedType*>(this->types[const_cast<Type*>(k.first)]), new ::skill::api::Map<::sir::FieldLike*, ::skill::api::String>()});
									tmp = fields->find(static_cast<sir::UserdefinedType*>(this->types[const_cast<Type*>(k.first)]));
								}

								(*(tmp->second))[this->field[const_cast<Field*>(j.first)]] = s;
							}
						}
					}
				}
				this->saveToolData.clear();
				this->saveTools.clear();

				// TODO: Types!
				this->sf->flush();
			}
	};
}

namespace sirEdit::data {
	std::unique_ptr<Serializer> getSir(std::string path) {
		return std::move(std::make_unique<SerializerSIR>(path));
	}
}
