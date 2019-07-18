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
	class Transactions; /// History
	/**
	 * Abstract interface to implement a serializer
	 *
	 * How to init: Read the specification of Field, FieldType, FieldMeta and Type for more information how to init them.
	 * 1. Load the data into your interal representation
	 * 2. Call updateRelationships to orginize things and update references with you didn't inited
	 *
	 * How to save function:
	 * 1. You get a prepareSave call.
	 * 2. You get a save call. (Dosen't have to be in the same thread but you get a memory fence).
	 * It's ensured, that first a prepareSave and then a save call is coming and that no of both functions will be called, as long as one of them is still running.
	 */
	class Serializer {
		private:
			std::vector<Type*> __types;     /// Known types
			std::vector<Type*> __baseTypes; /// Known base types
			std::vector<Tool*> __tools;     /// Known tools

			/**
			 * Set suptype information for super an implementations
			 * @param type that should updated to the other types
			 */
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

			/**
			 * Serializer implementation to add a type.
			 * @param type the type to add
			 */
			virtual void addBaseType(Type* type) = 0;
			/**
			 * Serializer implementation to remove a type. It have to delete the type from the memory!
			 * @param type the type to remove
			 */
			virtual void removeBaseType(Type* type) = 0;
			/**
			 * Serializer implementation to add a tool.
			 * @param tool the tool to add
			 */
			virtual void addBaseTool(Tool* tool) = 0;
			/**
			 * Serializer implementation to remove a tool. It have to delete the tool from the memory!
			 * @param tool the tool to remove
			 */
			virtual void removeBaseTool(Tool* tool) = 0;
			/**
			 * Serializer implementation to output all user types. It calls the given function with user type as paramter.
			 * @param callbackFunc the function the writes the info to ins internal representation.
			 */
			virtual void getBaseTypes(std::function<void(Type*)> callbackFunc) = 0;
			/**
			 * Serializer implementation to output all tools. It calls the given function with the tool as paramter.
			 * @param callbackFunc the function that writes the information into it's own representation.
			 */
			virtual void getBaseTools(std::function<void(Tool*)> callbackFunc) = 0;
		public:
			/**
			 * Default constructor. Is offen verry bussy calculate a proof of P = NP, more not.
			 */
			Serializer() {}
			/**
			 * Virtual deconstructor so that the implementation deconstructor get called. And it's removes the P = NP proof globaly (earth).
			 */
			virtual ~Serializer() {}

			/**
			 * Adds a type to the specification
			 * @param type the type to add
			 */
			void addType(Type* type) {
				this->__types.push_back(type);
				this->updateTypeInfo(type);
				this->addBaseType(type);
			}
			/**
			 * Removes a type from the specification. Important, the type and it's fields will be deleted after this function call!
			 * @param type the type to remove.
			 */
			void removeType(Type* type) {
				this->__types.erase(std::find(this->__types.begin(), this->__types.end(), type));
				this->removeBaseType(type);
			}
			/**
			 * Add a tool to the specification.
			 */
			void addTool(Tool* tool) {
				this->__tools.push_back(tool);
				this->addBaseTool(tool);
			}
			/**
			 * Remove a tool from the specification.
			 * @param tool the tool to remove.
			 */
			void removeTool(Tool* tool) {
				this->__tools.erase(std::find(this->__tools.begin(), this->__tools.end(), tool));
				this->removeBaseTool(tool);
			}
			/**
			 * Updates types when new types wer added or old one were removed.
			 */
			void update() {
				this->updateRelationships();
			}

			/**
			 * Prepare for saving. IMPORTANT: Be fast, you will mostly run in the gui thread!
			 */
			virtual void prepareSave() = 0;
			/**
			 * Save the data to the disk.
			 */
			virtual void save() = 0;

			/**
			 * Returns a list of known user types.
			 * @return list of known user types.
			 */
			const std::vector<Type*>& getTypes() const {
				return this->__types;
			}
			/**
			 * Returns a lit of known base user types.
			 * @return list of known user base types.
			 */
			const std::vector<Type*>& getBaseTypes() const {
				return this->__baseTypes;
			}
			/**
			 * Returns a list of known tools.
			 * @return list of known tools
			 */
			const std::vector<Tool*>& getTools() const {
				return this->__tools;
			}

			friend Transactions;
	};

	/**
	 * Create a sir serializer.
	 * @param path the path of the file to load
	 * @return the new sir serializer.
	 */
	extern std::unique_ptr<Serializer> getSir(std::string path);

	/**
	 * The historry of all done changes. TODO: !
	 */
	class Transactions {
		private:
			Serializer& __serializer;                                            /// The specification to use
			std::list<std::tuple<std::function<void(bool)>>> __history;          /// The saved history
			std::unordered_map<const Tool*, std::list<uint64_t>> __tool_history; /// Lookup for a tool witch history point it have to jump
			uint64_t __current_history = 0;                                      /// The current point in time (unfortunately not in space)
			std::vector<std::function<void()>> __change_callback;                /// Callbacks when something has changed

			/**
			 * Call the callbacks
			 * @tparam T Type of the list of callbacks
			 * @param the list of callbacks
			 */
			template<class T>
			void updateCall(T& list) {
				for(auto& i : list)
					i();
			}

			/**
			 * When a type is changed in a tool then run the callback to inform the gui of potential changes.
			 * This updates to the top, when something isn't readable any more.
			 * @param tool The tool witch is the target
			 * @param type The type that changed
			 * @param callback_type Callback witch helps to update views. So only called types has potenially changed. Not all
			 */
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

			/**
			 * Update the interface so that when you set/unset to deleting this types have to get udpates.
			 * @param tool The source tool
			 * @param type The type to check
			 * @param callback_type the callback function when this type has potentially changed.
			 */
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
			/**
			 * Creates a new history.
			 * @param serializer the specification to use
			 */
			Transactions(Serializer& serializer) : __serializer(serializer) {}

			/**
			 * Add a change callback to the list.
			 * @param func the function to add
			 */
			void addChangeCallback(std::function<void()> func) {
				this->__change_callback.push_back(std::move(func));
			}

			/**
			 * Returns the specification.
			 * @return serializer/specification
			 */
			const Serializer& getData() const {
				return this->__serializer;
			}

			/**
			 * Add a tool
			 * @param tool the tool to add
			 * @return the poitner to the new tool
			 */
			const Tool* addTool(Tool tool) {
				Tool* tmp = new Tool(std::move(tool));
				this->__serializer.addTool(tmp);
				updateCall(this->__change_callback);
				return tmp;
			}
			/**
			 * Remove a tool
			 * @param tool the tool to remove
			 */
			void removeTool(const Tool& tool) {
				this->__serializer.removeTool(const_cast<Tool*>(&tool));
				updateCall(this->__change_callback);
			}
			/**
			 * Updates the tool gernal data
			 * @param tool the tool to update
			 * @param name the name of the tool to change
			 * @param description the description of the tool to change
			 * @param cmd the command line to change
			 */
			void updateTool(const Tool& tool, std::string name, std::string description, std::string cmd) {
				const_cast<Tool*>(&tool)->getName() = std::move(name);
				const_cast<Tool*>(&tool)->getDescription() = std::move(description);
				const_cast<Tool*>(&tool)->getCommand() = std::move(cmd);
				updateCall(this->__change_callback);
			}
			/**
			 * Imports tools
			 * @param tools the tools to import
			 */
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
			/**
			 * Set a field in a tool to a specifc value
			 * @param tool the target tool
			 * @param type the type that was used
			 * @param field the field that gets changed
			 * @param state the state of the field
			 * @parma callback_field a callack for the gui to update the specifc field
			 * @param callback_type a callback for the gui to update a specific type
			 */
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
			/**
			 * Set a type to a specific status.
			 * @param tool The tool witch is the target
			 * @param type The type that changed
			 * @param state the new state of the type
			 * @param callback_type Callback witch helps to update views. So only called types has potenially changed. Not all
			 */
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
