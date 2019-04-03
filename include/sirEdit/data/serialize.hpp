#pragma once
#include <algorithm>
#include <functional>
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
			const std::vector<Type*>& getTypes() const;
			const std::vector<Type*>& getBaseTypes() const;
			const std::vector<Tool>& getTools() const;

			View addTool(Tool tool) const;

			bool saveToFile();

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
			~HistoricalView();

			const View& getStaticView() const {
				return this->staticView;
			}

			void addTool(Tool tool);
			void setFieldStatus(const Tool& tool, const Type& type, const Field& field, FIELD_STATE state, const std::function<void(const Type&, const Field&, FIELD_STATE, FIELD_STATE)>& callback_field, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type);
			void setTypeStatus(const Tool& tool, const Type& type, TYPE_STATE state, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type);
	};
}
