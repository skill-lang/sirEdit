#pragma once
#include <string>
#include <tuple>
#include <unordered_map>

#include <sirEdit/data/fields.hpp>

namespace sirEdit::data {
	class Tool {
		private:
			std::string name;

		public:
			Tool() {}
			Tool(std::string name) : name(std::move(name)) {}

			std::string& getName() { return this->name; }
			const std::string& getName() const { return this->name; }
	};
}
