#pragma once
#include <string>
#include <TypesOfType.h>


inline std::string getName(sir::UserdefinedType& type) {
	std::string result;
	for(int i = 0; i < type.getName()->getParts()->size(); i++)
		result += type.getName()->getParts()->get(i).string->c_str();
	return result;
}
