#pragma once
#include <memory>
#include <string>
#include <vector>

#include <sirEdit/data/types.hpp>
#include <sirEdit/data/tools.hpp>


namespace sirEdit::data {
	class Serializer;
	class View {
		private:
			std::shared_ptr<void> __raw;

			View(std::shared_ptr<void> raw);

		public:
			const std::vector<std::unique_ptr<Type>>& getTypes() const;
			const std::vector<Type*>& getBaseTypes() const;
			const std::vector<Tool>& getTools() const;

			View addTool(Tool tool) const;

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

	class HistoricalView {
		private:
			View staticView;
			std::shared_ptr<void> data;

		public:
			HistoricalView(View view);

			const View& getStaticView() const {
				return this->staticView;
			}

			void addTool(Tool tool);
	};
}
