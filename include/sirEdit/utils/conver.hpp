#pragma once
#include <exception>
#include <memory>
#include <sirEdit/data/types.hpp>
#include <TypesOfType.h>
#include <unordered_map>

namespace sirEdit::utils {
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
	inline std::unique_ptr<sirEdit::data::Type> genBaseType(sir::UserdefinedType* uf) {
		std::string skillType = uf->skillName();
		std::unique_ptr<sirEdit::data::Type> result;
		if(skillType == sir::ClassType::typeName) {
			std::unique_ptr<sirEdit::data::TypeWithFields> fields = std::move(_loadFields(static_cast<sir::ClassType*>(uf)));
			result = std::move(std::make_unique<sirEdit::data::TypeClass>(*(fields.get()), std::vector<sirEdit::data::TypeInterface*>(), nullptr));
		}
		else if(skillType == sir::InterfaceType::typeName) {
			std::unique_ptr<sirEdit::data::TypeWithFields> fields = std::move(_loadFields(static_cast<sir::InterfaceType*>(uf)));
			result = std::move(std::make_unique<sirEdit::data::TypeInterface>(*(fields.get()), std::vector<sirEdit::data::TypeInterface*>(), nullptr));
		}
		else
			throw std::invalid_argument(std::string("Unknown skill class type ") + skillType);
		return result;
	}

	template<class TARGET, class SOURCE>
	inline void _addSuper(TARGET* target, SOURCE* source, std::unordered_map<sir::UserdefinedType*, sirEdit::data::Type*>& types) {
		size_t counter = 0;
		target->getInterfaces().resize(source->getInterfaces()->size());
		for(sir::InterfaceType* i : *(source->getInterfaces())) {
			auto tmp = types.find(i);
			if(tmp == types.end())
				throw std::out_of_range("Type no in types list.");
			{
				auto tmp2 = dynamic_cast<sirEdit::data::TypeInterface*>(tmp->second);
				target->getInterfaces()[counter] = tmp2;
			}
			counter++;
		}
	}
	template<class TARGET, class SOURCE>
	inline void _addInterfaces(TARGET* target, SOURCE* source, std::unordered_map<sir::UserdefinedType*, sirEdit::data::Type*>& types) {
		if(target->getSuper() != nullptr) {
			auto tmp = types.find(source->getSuper());
			if(tmp == types.end())
				throw std::out_of_range("Type no in types list.");
			{
				auto tmp2 = dynamic_cast<sirEdit::data::TypeClass*>(tmp->second);
				target->getSuper() = tmp2;
			}
		}
	}
	template<class TARGET, class SOURCE>
	inline void _addReferences(sirEdit::data::Type* target, sir::UserdefinedType* source, std::unordered_map<sir::UserdefinedType*, sirEdit::data::Type*>& types) {
		TARGET* typeClass = dynamic_cast<TARGET*>(target);
		SOURCE* classType = static_cast<SOURCE*>(source);

		// Set super and interfaces
		_addSuper(typeClass, classType, types);
		_addInterfaces(typeClass, classType, types);
	}
	inline void updateBaseType(sirEdit::data::Type* target, sir::UserdefinedType* source, std::unordered_map<sir::UserdefinedType*, sirEdit::data::Type*>& types) {
		std::string skillType = source->skillName();
		if(skillType == sir::ClassType::typeName) {
			_addReferences<sirEdit::data::TypeClass, sir::ClassType>(target, source, types);
		}
		else if(skillType == sir::InterfaceType::typeName) {
			_addReferences<sirEdit::data::TypeInterface, sir::InterfaceType>(target, source, types);
		}
		else
			throw std::invalid_argument(std::string("Unknown skill class type ") + skillType);
	}
}
