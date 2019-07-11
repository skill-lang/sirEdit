#include <cstdio>
#include <File.h>
#include <TypesOfType.h>
#include <fstream>
#include <sirEdit/main.hpp>
#include <sirEdit/data/serialize.hpp>
#include <sirEdit/data/specUpdater.hpp>
//#include <sirEdit/utils/conver.hpp>
#include <unordered_map>
#include <tuple>
#include <functional>
#include <stdexcept>

#include <iostream>

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
	auto result = parseString(tmp, " ");

	// Tags
	if constexpr(false) {// TODO: Current buggy
		if(comment->getTags() != nullptr)
			for(auto& i : *(comment->getTags())) {
				if(i == nullptr)
					continue;
				result += string("\n@") + *(i->getName());
				if(i->getText() != nullptr)
					for(auto& j : *(i->getText()))
						result += std::string(" ") + *j;
			}
	}
	return result;
}

template<class FUNC>
inline void findFieldInSIR(sir::Type* type, FUNC func) {
	std::string skillType = type->skillName();
	if(skillType == sir::ClassType::typeName)
		func(static_cast<sir::ClassType*>(type)->getFields());
	else if(skillType == sir::InterfaceType::typeName)
		func(static_cast<sir::InterfaceType*>(type)->getFields());
	else if(skillType == sir::EnumType::typeName)
		func(static_cast<sir::EnumType*>(type)->getFields());
	else
		throw std::invalid_argument(std::string("Unknown skill class type ") + skillType);
}

template<class SOURCE>
inline sirEdit::data::TypeWithFields _loadFields(SOURCE* source) {
	if(source == nullptr)
		throw std::invalid_argument("Source is null pointer");
	std::vector<sirEdit::data::Field> fields;
	fields.resize(source->getFields()->size());
	size_t counter = 0;
	for(sir::FieldLike* i : *(source->getFields())) {
		fields[counter] = sirEdit::data::Field(parseString(i->getName()->getParts()), parseComment(i->getComment()), {});
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
	return sirEdit::data::TypeWithFields(parseString(source->getName()->getParts()), parseComment(source->getComment()), std::move(fields));
}
inline sirEdit::data::Type* genBaseType(sir::UserdefinedType& uf) {
	std::string skillType = uf.skillName();
	sirEdit::data::Type* result;

	// Create type
	if(skillType == sir::ClassType::typeName) {
		sirEdit::data::TypeWithFields fields = _loadFields(static_cast<sir::ClassType*>(&uf));
		result = new TypeClass(std::move(fields), std::vector<sirEdit::data::TypeInterface*>(), nullptr);
	}
	else if(skillType == sir::InterfaceType::typeName) {
		sirEdit::data::TypeWithFields fields = _loadFields(static_cast<sir::InterfaceType*>(&uf));
		result = new TypeInterface(std::move(fields), std::vector<sirEdit::data::TypeInterface*>(), nullptr);
	}
	else if(skillType == sir::EnumType::typeName) {
		sirEdit::data::TypeWithFields fields = _loadFields(static_cast<sir::EnumType*>(&uf));
		result = new TypeEnum(std::move(fields), nullptr);
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
inline void updateField(Field& field, unordered_map<Field*, sir::FieldLike*>& fields, unordered_map<sir::FieldLike*, Field*>& fieldInverse, unordered_map<sir::Type*, Type*>& typeInverse) {
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
	fieldInverse[orignalField] = &field;

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
			auto newSirComment(const std::string& comment) {
				auto result = this->sf->Comment->add();
				auto tmp = new skill::api::Array<skill::api::String>(1);
				(*tmp)[0] = this->newSirString(comment);
				result->setText(tmp);
				return result;
			}
			sir::Identifier* newSirIdentifier(const std::string& source) {
				auto name = this->newSirString(source);
				auto part = new skill::api::Array<::skill::api::String>(1);
				(*part)[0] = name;
				auto result = this->sf->Identifier->add();
				result->setSkillname(name);
				result->setParts(part);
				return result;
			}
			template<class SOURCE, class TARGET>
			void doDefault(SOURCE* source, TARGET* target) {
				// Add name and comment
				target->setName(this->newSirIdentifier(source->getName()));
				target->setComment(this->newSirComment(source->getComment()));

				// Add hints
				{
					auto tmp = new skill::api::Array<sir::Hint*>(0);
					target->setHints(tmp);
					for(auto& i : source->getHints()) {
						auto tmp2 = this->sf->Hint->add();
						tmp->push_back(tmp2);
						tmp2->setName(this->newSirString(i.first));
						auto tmp3 = new skill::api::Array<skill::api::String>(0);
						tmp2->setArguments(tmp3);
						for(auto& j : i.second)
							tmp3->push_back(this->newSirString(j));
					}
				}

				// Add restrictions
				{
					auto tmp = new skill::api::Array<sir::Restriction*>(0);
					target->setRestrictions(tmp);
					for(auto& i : source->getRestrictions()) {
						auto tmp2 = this->sf->Restriction->add();
						tmp->push_back(tmp2);
						tmp2->setName(this->newSirString(i.first));
						auto tmp3 = new skill::api::Array<skill::api::String>(0);
						tmp2->setArguments(tmp3);
						for(auto& j : i.second)
							tmp3->push_back(this->newSirString(j));
					}
				}
			}

			sir::Type* addSirType(Type* type) {
				// Check if instance allready exists
				{
					auto tmp = this->types.find(type);
					if(tmp != this->types.end())
						if(tmp->second != nullptr)
							return tmp->second;
				}

				// Add sir type
				sir::Type* result = doBaseType(type, [this, type]() -> sir::Type* { // Base
					return this->sf->SimpleType->add(this->newSirIdentifier(type->getName()));
				}, [this, type]() -> sir::Type* { // Interface
					auto result = this->sf->InterfaceType->add();
					this->doDefault(type, result);
					return result;
				}, [this, type]() -> sir::Type* { // Class
					auto result = this->sf->ClassType->add();
					this->doDefault(type, result);
					return result;
				}, [this, type]() -> sir::Type* { // Enum
					auto result = this->sf->EnumType->add();

					// Add enum instances
					{
						auto tmp = new skill::api::Array<::sir::Identifier*>(0);
						result->setInstances(tmp);
						for(auto& i : static_cast<TypeEnum*>(type)->getFields())
							if(i.getMeta().type == FieldMeta::META_TYPE::ENUM_INSTANCE)
								tmp->push_back(this->newSirIdentifier(i.getName()));
					}
					return result;
				}, [this, type]() -> sir::Type* { // Typedef
					auto result = this->sf->TypeDefinition->add();
					this->doDefault(type, result);
					return result;
				});
				this->types[type] = result;
				return result;
			}

			void updateType(Type* type) {
				sir::Type* sirType = this->types[type];
				// Update content
				doBaseType(type, []() -> void {}, [this, type, sirType]() -> void { // Interface
					// Set super
					auto tmp = dynamic_cast<sir::InterfaceType*>(sirType);
					tmp->setSuper(dynamic_cast<sir::ClassType*>(this->addSirType(const_cast<Type*>(getSuper(*type)))));
				}, [this, type, sirType]() -> void { // Class
					// Set super
					auto tmp = dynamic_cast<sir::ClassType*>(sirType);
					tmp->setSuper(dynamic_cast<sir::ClassType*>(this->addSirType(const_cast<Type*>(getSuper(*type)))));

					// Set implementations
					auto tmp2 = new skill::api::Set<sir::InterfaceType*>(0);
					tmp->setInterfaces(tmp2);
				}, []() -> void {}, [this, type, sirType]() -> void { // Typedef
					auto tmp = dynamic_cast<sir::TypeDefinition*>(sirType);
					tmp->setTarget(dynamic_cast<sir::ClassType*>(this->addSirType(const_cast<Type*>(getSuper(*type)))));
				});
			}

		public:
			SerializerSIR(std::string path) {
				// Open file
				if(std::ifstream(path).good())
					this->sf = SkillFile::open(path);
				else
					this->sf = SkillFile::create(path);
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
						updateField(j, this->field, this->fieldInverse, this->typesInverse);
				}

				// Field inverse pass
				for(auto& i : this->field)
					this->fieldInverse[i.second] = i.first;

				// Read tools
				for(auto& i : this->sf->Tool->all()) {
					Tool* tmpTool = new Tool();
					tmpTool->getName() = std::move(std::string(i.getName()->begin(), i.getName()->end()));
					tmpTool->getDescription() = std::move(std::string(i.getDescription()->begin(), i.getDescription()->end()));
					tmpTool->getCommand() = std::move(std::string(i.getCommand()->begin(), i.getCommand()->end()));

					// Load type states
					for(auto& j : *(i.getSelTypes())) {
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
					std::cout << "Fields :" << this->fieldInverse.size() << std::endl;
					for(auto& j : *(i.getSelFields())) {
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

				// Clear up types
				for(auto& i : this->types)
					delete i.first;

				// Clear up tools
				for(auto& i : this->tools)
					delete i.first;
			}

			void addBaseType(Type* type) {
				this->addType.push_back(type);
				this->types[type] = nullptr;
			}
			void removeBaseType(Type* type) {
				throw; // TODO:
			}
			void addBaseTool(Tool* tool) {
				this->toAdd.push_back(tool);
				this->tools[tool] = nullptr;
			}
			void removeBaseTool(Tool* tool) {
				this->toRemove.push_back(tools[tool]);
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
					if(this->tools.count(i) != 0) {
						// Add tool
						auto tool = this->sf->Tool->add();
						this->tools[i] = tool;

						// Add buildtarget skill
						auto bi = new skill::api::Array<sir::BuildInformation*>(1);
						tool->setBuildTargets(bi);
						auto skill = this->sf->BuildInformation->add();
						(*bi)[0] = skill;
						skill->setLanguage(this->newSirString("skill"));
						auto skillOutput = this->sf->FilePath->add();
						auto skillOutputPath = new skill::api::Array<skill::api::String>(1);
						(*skillOutputPath)[0] = this->newSirString(".");
						skillOutput->setParts(skillOutputPath);
						skillOutput->setIsAbsolut(false);
						skill->setOutput(skillOutput);
						auto options = new skill::api::Array<skill::api::String>(2);
						(*options)[0] = this->newSirString("--package");
						(*options)[1] = this->newSirString("package");
						skill->setOptions(options);

						// Fix bugs
						tool->setCustomTypeAnnotations(new skill::api::Map<sir::UserdefinedType*, sir::ToolTypeCustomization*>());
						tool->setCustomFieldAnnotations(new skill::api::Map<sir::FieldLike*, sir::ToolTypeCustomization*>());
					}
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

				// Add types
				for(auto& i : this->addType)
					this->addSirType(i);
				// TODO: Remove types
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
					if(sTool->getSelTypes() !=  nullptr)
						delete sTool->getSelTypes();
					sTool->setSelTypes(types);

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
						if(sTool->getSelFields() != nullptr)
							delete sTool->getSelFields();
						sTool->setSelFields(fields);

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

								auto& tmp2 = (*fields)[static_cast<sir::UserdefinedType*>(this->types[const_cast<Type*>(k.first)])];
								(*tmp2)[this->field[const_cast<Field*>(j.first)]] = s;
							}
						}
					}

					// Update codegen data
					{
						if(sTool->getSelectedUserTypes() == nullptr)
							sTool->setSelectedUserTypes(new skill::api::Set<sir::UserdefinedType*>(0));
						if(sTool->getSelectedFields() == nullptr)
							sTool->setSelectedFields(new skill::api::Map<sir::UserdefinedType*, skill::api::Map<skill::api::String, sir::FieldLike*>*>());
						auto& forCMDType = *(sTool->getSelectedUserTypes());
						forCMDType.clear();
						auto& forCMDField = *(sTool->getSelectedFields());
						forCMDField.clear();

						// Bugfix for interfaces
						for(auto& j : this->types) {
							if(i.second.getTypeTransitiveState(*j.first) < TYPE_STATE::READ)
								continue;
							auto nothing = []() -> void {};
							doBaseType(j.first, nothing, [&i, j]() -> void {
								for(auto& k : getFields(*j.first))
									i.second.setFieldState(*j.first, k, FIELD_STATE::READ);
							}, nothing, nothing, nothing);
						}

						// Set data
						for(auto j : this->types)
							if(dynamic_cast<sir::UserdefinedType*>(j.second) != nullptr)
								if(i.second.getTypeTransitiveState(*j.first) >= TYPE_STATE::READ) {
									forCMDType.insert(dynamic_cast<sir::UserdefinedType*>(j.second));
									auto fieldsRef = new skill::api::Map<skill::api::String, sir::FieldLike*>();
									forCMDField[dynamic_cast<sir::UserdefinedType*>(j.second)] = fieldsRef;
									for(auto& k : listAllFields(*(j.first)))
										if(i.second.getFieldTransitiveState(*k) >= FIELD_STATE::READ) {
											auto tmp = this->field.find(const_cast<Field*>(k));
											if(tmp != this->field.end())
												(*fieldsRef)[this->newSirString(j.first->getName())] = tmp->second;
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

extern std::string sirEdit::data::SpecModify::dropToSIR() {
	string sirFile = string(tmpnam(nullptr)) + ".sir";
	auto serializer = sirEdit::data::getSir(sirFile);
	for(auto& i : this->types)
		serializer->addType(i);
	serializer->prepareSave();
	serializer->save();
	return sirFile;
}
extern std::string sirEdit::data::SpecModify::dropToSKilL() {
	// Create path
	string path = string(tmpnam(nullptr));
	mkdir(path.c_str(), 0777);

	// Export sir
	{
		auto serializer = sirEdit::data::getSir(path + "/sir.sir");
		for(auto& i : this->types)
			serializer->addType(i);
		serializer->prepareSave();
		serializer->save();
	}

	// Generate specification
	runCodegen({"sir.sir", "--add-tool", "export"}, path);
	runCodegen({"sir.sir", "-t", "export", "--select-types", "*"}, path);
	runCodegen({"sir.sir", "-t", "export", "--add-target", "skill . --package skill"}, path);
	return path + "/specification.skill";
}

namespace sirEdit::data {
	std::unique_ptr<Serializer> getSir(std::string path) {
		return std::move(std::make_unique<SerializerSIR>(path));
	}
}
