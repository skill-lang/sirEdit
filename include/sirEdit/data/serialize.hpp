#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <sirEdit/data/types.hpp>
#include <sirEdit/data/tools.hpp>


namespace sirEdit::data {
	class Transactions;
	class Serializer {
		private:
			std::vector<Type*> types;
			std::vector<Type*> baseTypes;
			std::vector<Tool*> tools;

			void updateTypeInfo(Type* type) {
				Type* tmp = const_cast<Type*>(getSuper(*type));
				if(tmp != nullptr)
					tmp->getSubTypes().push_back(type);
				else
					this->baseTypes.push_back(type);
			}

		protected:
			void updateRelationships() {
				// Get data
				{
					this->types.clear();
					this->getBaseTypes([this](Type* type) -> void {
						this->types.push_back(type);
						type->getSubTypes().clear();
					});
					this->tools.clear();
					this->getBaseTools([this](Tool* tool) -> void {
						this->tools.push_back(tool);
					});
				}

				// Create new subtype information
				for(auto& i : this->types)
					this->updateTypeInfo(i);
			}

			virtual void addBaseType(Type* type) = 0;
			virtual void addBaseTool(Tool* tool) = 0;
			virtual void getBaseTypes(std::function<void(Type*)> callbackFunc) = 0;
			virtual void getBaseTools(std::function<void(Tool*)> callbackFunc) = 0;
		public:
			Serializer() {}
			virtual ~Serializer() {}

			void addType(Type* type) {
				this->types.push_back(type);
				this->updateTypeInfo(type);
				this->addBaseType(type);
			}
			void addTool(Tool* tool) {
				this->tools.push_back(tool);
				this->addBaseTool(tool);
			}
			virtual void save() = 0;

			const std::vector<Type*>& getTypes() const {
				return this->types;
			}
			const std::vector<Type*>& getBaseTypes() const {
				return this->baseTypes;
			}
			const std::vector<Tool*>& getTools() const {
				return this->tools;
			}

			friend Transactions;
	};

	extern std::unique_ptr<Serializer> getSir(std::string path);

	class Transactions {
		private:
			Serializer& serializer;
			std::vector<std::function<void()>> change_callback;

			template<class T>
			inline void updateCall(T& list) {
				for(auto& i : list)
					i();
			}

			void updateSubtypes(const Tool& tool, const Type& type, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
				for(auto& i : type.getSubTypes()) {
					auto tmp = tool.getTypeTransitiveState(type);
					if(tmp >= TYPE_STATE::READ) {
						callback_type(type, tmp, tool.getTypeSetState(type));
						updateSubtypes(tool, *i, callback_type);
					}
				}
			}

		public:
			Transactions(Serializer& serializer) : serializer(serializer) {}

			void addChangeCallback(std::function<void()> func) {
				this->change_callback.push_back(std::move(func));
			}

			const Serializer& getData() const {
				return this->serializer;
			}

			void addTool(Tool tool) {
				this->serializer.addTool(new Tool(std::move(tool)));
				updateCall(this->change_callback);
			}
			void setFieldStatus(const Tool& tool, const Type& type, const Field& field, FIELD_STATE state, const std::function<void(const Type&, const Field&, FIELD_STATE, FIELD_STATE)>& callback_field, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
				// Set state
				FIELD_STATE old = tool.getFieldSetState(field, type);
				const_cast<Tool&>(tool).setFieldState(type, field, state);

				// Callback field
				{
					callback_field(type, field, tool.getFieldTransitiveState(field), state);
				}

				// Callback type
				{
					const Type* tmp_type = &type;
					while(tmp_type != nullptr) {
						callback_type(*tmp_type, tool.getTypeTransitiveState(*tmp_type), tool.getTypeSetState(*tmp_type));
						tmp_type = getSuper(*tmp_type);
					}
				}
				updateCall(this->change_callback);
			}
			void setTypeStatus(const Tool& tool, const Type& type, TYPE_STATE state, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
				// Set type state
				TYPE_STATE oldTrans = tool.getTypeTransitiveState(type);
				const_cast<Tool&>(tool).setTypeState(type, state);

				// Callback
				TYPE_STATE newTrans = tool.getTypeTransitiveState(type);
				callback_type(type, newTrans, state);
				{
					// Update super classes
					const Type* current = getSuper(type);
					while(current != nullptr) {
						callback_type(*current, tool.getTypeTransitiveState(*current), tool.getTypeSetState(*current));
						current = getSuper(*current);
					}
				}
				if(oldTrans == TYPE_STATE::DELETE || newTrans == TYPE_STATE::DELETE) {
					// Update sub classes
					updateSubtypes(tool, type, callback_type);
				}
				updateCall(this->change_callback);
			}
	};
}
