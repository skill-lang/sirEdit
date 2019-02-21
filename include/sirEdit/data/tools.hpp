#pragma once
#include <string>

namespace sirEdit::data {
	class Tool {
		private:
			std::string name;

		public:
			Tool(std::string name) : name(std::move(name)) {}

			std::string& getName() { return this->name; }
			const std::string& getName() const { return this->name; }
	};
}
