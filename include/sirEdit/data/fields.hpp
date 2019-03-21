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
			std::string name;
			std::string comment;
			std::string type;
			std::unordered_map<Tool*, std::unordered_map<Type*, FIELD_STATE>> __states;

		public:
			Field() {}
			Field(std::string name, std::string comment, std::string type) : name(name), comment(comment), type(type) {}

			const std::string& getName() const { return this->name; }
			const std::string& getComment() const { return this->comment; }
			const std::string& getType() const { return this->type; }
			const std::unordered_map<Tool*, std::unordered_map<Type*, FIELD_STATE>>& getStates() const { return this->__states; }
			std::string& getName() { return this->name; }
			std::string& getComment() { return this->comment; }
			std::string& getType() { return this->type; }
			std::unordered_map<Tool*, std::unordered_map<Type*, FIELD_STATE>>& getStates() { return this->__states; }

			FIELD_STATE getToolType(const Tool& tool) const {
				FIELD_STATE state = FIELD_STATE::UNUSED;
				auto tmp = this->__states.find(const_cast<Tool*>(&tool));
				if(tmp == this->__states.end())
					return state;
				for(auto& i : tmp->second)
					state = std::max(state, i.second);
				return state;
			}
	};
}
