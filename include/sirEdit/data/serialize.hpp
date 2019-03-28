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

			bool getTypeTransitive_subtypes(const Tool& tool, const Type& type) const {
				if(type.getToolSet(tool) > TYPE_STATE::UNUSED)
					return true;
				{
					auto tmp = type.getCountFieldsSet().find(const_cast<Tool*>(&tool));
					if(tmp != type.getCountFieldsSet().end() && tmp->second > 0)
						return true;
				}
				for(auto& i : type.getSubTypes())
					// TODO: Add cache
					if(this->getTypeTransitive_subtypes(tool, *i))
						return true;
				return false;
			}

			bool getTypeTransitive_parentDelete(const Tool& tool, const Type& type) const {
				const Type* tmp = &type;
				while(tmp != nullptr) {
					if(tmp->getToolSet(tool) == TYPE_STATE::DELETE)
						return true;
					tmp = getSuper(*tmp);
				}
				return false;
			}

		public:
			const std::vector<Type*>& getTypes() const;
			const std::vector<Type*>& getBaseTypes() const;
			const std::vector<Tool>& getTools() const;

			View addTool(Tool tool) const;

			bool saveToFile();

			TYPE_STATE getTypeSet(const Tool& tool, const Type& type) const {
				auto tmp = type.getState().find(const_cast<Tool*>(&tool));
				if(tmp == type.getState().end())
					return TYPE_STATE::NO;
				else
					return tmp->second;
			}
			TYPE_STATE getTypeTransitive(const Tool& tool, const Type& type) const {
				TYPE_STATE result = TYPE_STATE::UNUSED;
				// Mode set in tool
				{
					auto tmp = type.getState().find(const_cast<Tool*>(&tool));
					if(tmp != type.getState().end())
						result = std::max(result, tmp->second);
				}

				// Fields set in tool
				{
					auto tmp = type.getCountFieldsSet().find(const_cast<Tool*>(&tool));
					if(tmp != type.getCountFieldsSet().end())
						if(tmp->second != 0)
							result = std::max(result, TYPE_STATE::READ);
				}

				// Check child
				if(this->getTypeTransitive_subtypes(tool, type))
					result = std::max(result, TYPE_STATE::READ);

				// Check parents
				if(this->getTypeTransitive_parentDelete(tool, type))
					result = TYPE_STATE::DELETE;

				// Return result
				return result;
			}

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
			void setFieldStatus(const Tool& tool, Type& type, Field& field, FIELD_STATE state, const std::function<void(const Type&, const Field&, FIELD_STATE, FIELD_STATE)>& callback_field, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type);
			void setTypeStatus(const Tool& tool, Type& type, TYPE_STATE state, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type);
	};
}
