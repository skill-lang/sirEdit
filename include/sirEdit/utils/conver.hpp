#pragma once
#include <exception>
#include <memory>
#include <sirEdit/data/types.hpp>
#include <TypesOfType.h>
#include <unordered_map>

#include <iostream>

namespace sirEdit::utils {
	template<class TARGET, class SOURCE>
	inline void _addInterfaces(TARGET* target, SOURCE* source, std::unordered_map<sir::UserdefinedType*, sirEdit::data::Type*>& types) {
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
	inline void _addSuper(TARGET* target, SOURCE* source, std::unordered_map<sir::UserdefinedType*, sirEdit::data::Type*>& types, std::vector<sirEdit::data::Type*>& baseTypes) {
		if(target->getSuper() != nullptr) {
			auto tmp = types.find(source->getSuper());
			if(tmp == types.end())
				throw std::out_of_range("Type no in types list.");
			{
				auto tmp2 = dynamic_cast<sirEdit::data::TypeClass*>(tmp->second);
				target->getSuper() = tmp2;
				tmp2->getSubTypes().push_back(target);
			}
		}
		else {
			baseTypes.push_back(target);
		}
	}
	template<class TARGET, class SOURCE>
	inline void _addReferences(sirEdit::data::Type* target, sir::UserdefinedType* source, std::unordered_map<sir::UserdefinedType*, sirEdit::data::Type*>& types, std::vector<sirEdit::data::Type*>& baseTypes) {
		TARGET* typeClass = dynamic_cast<TARGET*>(target);
		SOURCE* classType = static_cast<SOURCE*>(source);

		// Set super and interfaces
		_addSuper(typeClass, classType, types, baseTypes);
		_addInterfaces(typeClass, classType, types);
	}
	inline void updateBaseType(sirEdit::data::Type* target, sir::UserdefinedType* source, std::unordered_map<sir::UserdefinedType*, sirEdit::data::Type*>& types, std::vector<sirEdit::data::Type*>& baseTypes) {
		std::string skillType = source->skillName();
		if(skillType == sir::ClassType::typeName) {
			_addReferences<sirEdit::data::TypeClass, sir::ClassType>(target, source, types, baseTypes);
		}
		else if(skillType == sir::InterfaceType::typeName) {
			_addReferences<sirEdit::data::TypeInterface, sir::InterfaceType>(target, source, types, baseTypes);
		}
		else
			throw std::invalid_argument(std::string("Unknown skill class type ") + skillType);
	}
}
