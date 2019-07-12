#pragma once
#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <sirEdit/data/types.hpp>
#include <sirEdit/data/tools.hpp>


namespace sirEdit::data {
	class Transactions;
	class Serializer {
		private:
			std::vector<Type*> __types;
			std::vector<Type*> __baseTypes;
			std::vector<Tool*> __tools;

			void updateTypeInfo(Type* type) {
				// Base types
				{
					Type* tmp = const_cast<Type*>(getSuper(*type));
					if(tmp != nullptr)
						tmp->getSubTypes().push_back(type);
					else
						this->__baseTypes.push_back(type);
				}

				// Interfaces
				for(auto& i : getInterfaces(*type))
					i->getSubTypes().push_back(type);
			}

		protected:
			void updateRelationships() {
				// Get data
				{
					this->__types.clear();
					this->getBaseTypes([this](Type* type) -> void {
						this->__types.push_back(type);
						type->getSubTypes().clear();
					});
					this->__tools.clear();
					this->getBaseTools([this](Tool* tool) -> void {
						this->__tools.push_back(tool);
					});
				}

				// Create new subtype information
				for(auto& i : this->__types)
					this->updateTypeInfo(i);

				// Calculating collide of types
				std::vector<Type*> onlySubtype; // Types that are at then end of the hirarchie
				std::unordered_map<std::string, std::vector<Type*>> collideInfo;
				for(auto& i : this->__types) {
					if(i->getSubTypes().size() == 0)
						onlySubtype.push_back(i);
					auto tmp = collideInfo.find(i->getName());
					if(tmp == collideInfo.end())
						collideInfo[i->getName()] = {i};
					else
						tmp->second.push_back(i);
				}
				for(auto& i : collideInfo) // Set colide info for types
					if(i.second.size() > 1)
						for(auto& j : i.second) {
							j->getCollides() = {i.second.begin(), i.second.end()};
							j->getCollides().erase(std::find(j->getCollides().begin(), j->getCollides().end(), j));
						}

				// Calculating collide of fields
				for(auto& i : onlySubtype) {
					std::unordered_map<std::string, std::vector<Field*>> fieldName;
					for(auto& j : listAllFields(*i)) { // Search for broken fields
						auto tmp = fieldName.find(j->getName());
						if(tmp == fieldName.end())
							fieldName[j->getName()] = {const_cast<Field*>(j)};
						else
							fieldName[j->getName()].push_back(const_cast<Field*>(j));
					}
					for(auto& j : fieldName) // Updte fields
						if(j.second.size() > 1)
							for(auto& k : j.second) {
								k->getCollide() = {j.second.begin(), j.second.end()};
								k->getCollide().erase(std::find(k->getCollide().begin(), k->getCollide().end(), k));
							}
				}
			}

			virtual void addBaseType(Type* type) = 0;
			virtual void removeBaseType(Type* type) = 0;
			virtual void addBaseTool(Tool* tool) = 0;
			virtual void removeBaseTool(Tool* tool) = 0;
			virtual void getBaseTypes(std::function<void(Type*)> callbackFunc) = 0;
			virtual void getBaseTools(std::function<void(Tool*)> callbackFunc) = 0;
		public:
			Serializer() {}
			virtual ~Serializer() {}

			void addType(Type* type) {
				this->__types.push_back(type);
				this->updateTypeInfo(type);
				this->addBaseType(type);
			}
			void removeType(Type* type) {
				throw; // TODO
			}
			void addTool(Tool* tool) {
				this->__tools.push_back(tool);
				this->addBaseTool(tool);
			}
			void removeTool(Tool* tool) {
				this->__tools.erase(std::find(this->__tools.begin(), this->__tools.end(), tool));
				this->removeBaseTool(tool);
			}
			void update() {
				this->updateRelationships();
			}
			virtual void prepareSave() = 0;
			virtual void save() = 0;

			const std::vector<Type*>& getTypes() const {
				return this->__types;
			}
			const std::vector<Type*>& getBaseTypes() const {
				return this->__baseTypes;
			}
			const std::vector<Tool*>& getTools() const {
				return this->__tools;
			}

			friend Transactions;
	};

	extern std::unique_ptr<Serializer> getSir(std::string path);

	class Transactions {
		private:
			Serializer& __serializer;
			std::list<std::tuple<std::function<void(bool)>>> __history;
			std::unordered_map<const Tool*, std::list<uint64_t>> __tool_history;
			uint64_t __current_history = 0;
			std::vector<std::function<void()>> __change_callback;

			template<class T>
			void updateCall(T& list) {
				for(auto& i : list)
					i();
			}

			void updateParentTypes(const Tool& tool, const Type& type, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
				// Update super classes
				const Type* current = &type;
				while(current != nullptr) {
					// Callback current type
					callback_type(*current, tool.getTypeTransitiveState(*current), tool.getTypeSetState(*current));

					// Update interfaces
					for(auto& i : getInterfaces(*current))
						updateParentTypes(tool, *i, callback_type);

					// Update current
					current = getSuper(*current);
				}
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
			Transactions(Serializer& serializer) : __serializer(serializer) {}

			void addChangeCallback(std::function<void()> func) {
				this->__change_callback.push_back(std::move(func));
			}

			const Serializer& getData() const {
				return this->__serializer;
			}

			const Tool* addTool(Tool tool) {
				Tool* tmp = new Tool(std::move(tool));
				this->__serializer.addTool(tmp);
				updateCall(this->__change_callback);
				return tmp;
			}
			void removeTool(const Tool& tool) {
				this->__serializer.removeTool(const_cast<Tool*>(&tool));
				updateCall(this->__change_callback);
			}
			void updateTool(const Tool& tool, std::string name, std::string description, std::string cmd) {
				const_cast<Tool*>(&tool)->getName() = std::move(name);
				const_cast<Tool*>(&tool)->getDescription() = std::move(description);
				const_cast<Tool*>(&tool)->getCommand() = std::move(cmd);
				updateCall(this->__change_callback);
			}
			void importTools(const std::vector<Tool*>& tools) {
				std::unordered_map<std::string, const Tool*> toolNames; // load tool names
				for(auto& i : this->__serializer.getTools())
					toolNames[i->getName()] = i;

				// Add tools
				for(auto& i : tools) {
					size_t counter = 1;
					while(true) {
						// Generate name for tool
						std::string name = i->getName();
						if(counter > 1)
							name += "(" + std::to_string(counter) + ")";

						// Try to insert tool
						auto tmp = toolNames.find(name);
						if(tmp == toolNames.end()) {
							toolNames[name] = i;
							Tool* tmp2 = new Tool(*i);
							tmp2->getName() = name;
							this->__serializer.addTool(tmp2);
							break;
						}
						else
							counter++;
					}
				}
				updateCall(this->__change_callback);
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
				for(auto& i : field.getType().types)
					this->updateParentTypes(tool, *i, callback_type);
				this->updateParentTypes(tool, type, callback_type);
				updateCall(this->__change_callback);
			}
			void setTypeStatus(const Tool& tool, const Type& type, TYPE_STATE state, const std::function<void(const Type&, TYPE_STATE, TYPE_STATE)>& callback_type) {
				// Set type state
				TYPE_STATE oldTrans = tool.getTypeTransitiveState(type);
				const_cast<Tool&>(tool).setTypeState(type, state);

				// Callback
				TYPE_STATE newTrans = tool.getTypeTransitiveState(type);
				this->updateParentTypes(tool, type, callback_type);
				if((oldTrans == TYPE_STATE::DELETE) != (newTrans == TYPE_STATE::DELETE)) {
					// Update sub classes
					updateSubtypes(tool, type, callback_type);
				}
				updateCall(this->__change_callback);
			}
	};
}
