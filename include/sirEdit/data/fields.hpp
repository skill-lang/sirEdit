#pragma once
#include <string>


namespace sirEdit::data {
	class Field {
		private:
			std::string name;
			std::string comment;
			std::string type;

		public:
			Field() {}
			Field(std::string name, std::string comment, std::string type) : name(name), comment(comment), type(type) {}

			const std::string& getName() const { return this->name; }
			const std::string& getComment() const { return this->comment; }
			const std::string& getType() const { return this->type; }
			std::string& getName() { return this->name; }
			std::string& getComment() { return this->comment; }
			std::string& getType() { return this->type; }
	};
}
