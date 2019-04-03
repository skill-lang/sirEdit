#pragma once
#include <string>
#include <unordered_map>


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
	class Field {
		private:
			std::string __name;
			std::string __comment;
			std::string __type;

		public:
			Field() {}
			Field(std::string name, std::string comment, std::string type) : __name(name), __comment(comment), __type(type) {}

			const std::string& getName() const { return this->__name; }
			const std::string& getComment() const { return this->__comment; }
			const std::string& getType() const { return this->__type; }
			std::string& getName() { return this->__name; }
			std::string& getComment() { return this->__comment; }
			std::string& getType() { return this->__type; }
	};
}
