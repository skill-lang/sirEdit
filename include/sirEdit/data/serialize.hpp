#pragma once
#include <memory>
#include <string>
#include <vector>

#include <sirEdit/data/types.hpp>


namespace sirEdit::data {
	class Serializer;
	class View {
		private:
			std::shared_ptr<void> __raw;

			View(std::shared_ptr<void> raw) : __raw(std::move(raw)) {}

		public:
			const std::vector<std::unique_ptr<Type>>& getTypes();
			const std::vector<Type*>& getBaseTypes();

			friend sirEdit::data::Serializer;
	};

	class Serializer {
		private:
			std::shared_ptr<void> __raw_data;

			Serializer(std::shared_ptr<void> data) : __raw_data(std::move(data)) {}

		public:
			Serializer(const Serializer&) = delete;
			Serializer& operator =(const Serializer&) = delete;

			View getView() { return View(this->__raw_data); }

			static std::shared_ptr<Serializer> openFile(const std::string& file);
	};
}
