#pragma once
#include <string>
#include <unordered_map>
#include <vector>


namespace sirEdit::data {
	enum struct FIELD_STATE {
		NO,
		UNUSED,
		READ,
		WRITE,
		CREATE
	};

	class Tool;
	class Type;
	struct FieldType {
		enum struct TYPE_COMBINATION {
			SINGLE_TYPE,
			DYNAMIC_ARRAY,
			STATIC_ARRAY,
			LIST,
			SET,
			MAP
		};

		std::vector<Type*> types;
		TYPE_COMBINATION combination;
		uint64_t arraySize;
	};
	class Field {
		private:
			std::string __name;
			std::string __comment;
			FieldType __type;
			std::unordered_map<std::string, std::vector<std::string>> __hints;
			std::unordered_map<std::string, std::vector<std::string>> __restrictions;

		public:
			Field() {}
			Field(std::string name, std::string comment, FieldType type) : __name(std::move(name)), __comment(std::move(comment)), __type(std::move(type)) {}

			const std::string& getName() const { return this->__name; }
			const std::string& getComment() const { return this->__comment; }
			const FieldType& getType() const { return this->__type; }
			const std::unordered_map<std::string, std::vector<std::string>>& getHints() const { return this->__hints; }
			const std::unordered_map<std::string, std::vector<std::string>>& getRestrictions() const { return this->__restrictions; }
			std::string& getName() { return this->__name; }
			std::string& getComment() { return this->__comment; }
			FieldType& getType() { return this->__type; }
			std::unordered_map<std::string, std::vector<std::string>>& getHints() { return this->__hints; }
			std::unordered_map<std::string, std::vector<std::string>>& getRestrictions() { return this->__restrictions; }

			std::string printType() const;
	};
}

#include <sirEdit/data/types.hpp>
