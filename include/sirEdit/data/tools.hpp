#pragma once
#include <algorithm>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include <sirEdit/data/fields.hpp>
#include <sirEdit/data/types.hpp>

namespace sirEdit::data {
	/**
	 * A tool
	 */
	class Tool {
		private:
			std::string __name;        /// The name of the tool
			std::string __description; /// The description of the tool
			std::string __command;     /// The command to build the tool

			std::unordered_map<const Field*, std::unordered_map<const Type*, FIELD_STATE>> __statesFields; /// the state of fields
			std::unordered_map<const Type*, std::tuple<uint64_t, TYPE_STATE>> __statesType;                /// the state of types and references

			//
			// Calc transetive states
			//
			mutable std::unordered_map<const Field*, FIELD_STATE> __cache_fields; /// Cache for fields
			mutable std::unordered_map<const Type*, bool> __cache_types_delete;   /// Cache delete state for types
			mutable std::unordered_map<const Type*, TYPE_STATE> __cache_types;    /// Cache state of types

			/**
			 * Calculate the transitive state of a field.
			 * @param field the field to calculate
			 * @return state of the field
			 */
			FIELD_STATE __transField(const Field* field) const {
				// Cache hit?
				{
					auto tmp = this->__cache_fields.find(field);
					if(tmp != this->__cache_fields.end())
						return tmp->second;
				}

				// New result
				FIELD_STATE result = FIELD_STATE::UNUSED;

				// Check if value is set
				{
					auto tmpField = this->__statesFields.find(field);
					if(tmpField != this->__statesFields.end()) {
						for(auto& i : tmpField->second)
							result = std::max(result, i.second);
					}
				}

				// Check views
				for(auto& i : field->getMeta().viewInverse)
					result = std::max(result, this->__transField(i));

				// Add to cache and return
				this->__cache_fields[field] = result;
				return result;
			}

			/**
			 * Calculating if a type is transetive to deleted
			 * @param type the type to check
			 * @return is type recusive set to deleted
			 */
			bool __isDelete(const Type* type) const {
				// Cache hit?
				{
					auto tmp = this->__cache_types_delete.find(type);
					if(tmp != this->__cache_types_delete.end())
						return tmp->second;
				}

				// Calculating result, init
				bool result = false;

				// Check if type is self set
				{
					auto tmp = this->__statesType.find(type);
					if(tmp != this->__statesType.end())
						if(std::get<1>(tmp->second) == TYPE_STATE::DELETE)
							result |= true;
				}

				// Check super type
				if(!result) {
					const Type* super = getSuper(*type);
					if(super != nullptr)
						result |= this->__isDelete(super);
				}

				// Check interfaces
				if(!result) {
					auto& interfaces = getInterfaces(*type);
					for(auto& i : interfaces) {
						result |= this->__isDelete(i);
						if(result) // Speedup
							break;
					}
				}

				// Cache result and return
				this->__cache_types_delete[type] = result;
				return result;
			}

			/**
			 * Calculates the transitive state of a type
			 * @param type the type to calculate
			 * @return state of the type
			 */
			TYPE_STATE __transType(const Type* type) const {
				// Cache hit?
				{
					auto tmp = this->__cache_types.find(type);
					if(tmp != this->__cache_types.end())
						return tmp->second;
				}

				// Calculating new result, init
				TYPE_STATE result = TYPE_STATE::UNUSED;

				// Check if state is set and field or refernce from a field is given
				{
					auto tmp = this->__statesType.find(type);
					if(tmp != this->__statesType.end()) {
						result = std::max(result, std::get<1>(tmp->second));
						if(std::get<0>(tmp->second))
							result = std::max(result, TYPE_STATE::READ);
					}
				}

				// Check if own fields are set on created -> set type to write
				for(auto& i : getFields(*type)) {
					FIELD_STATE tmp = this->__transField(&i);
					if(tmp == FIELD_STATE::CREATE)
						result = std::max(result, TYPE_STATE::WRITE);
					else if(tmp >= FIELD_STATE::READ)
						result = std::max(result, TYPE_STATE::READ);

					if(result >= TYPE_STATE::WRITE) // Speedup
						break;
				}

				// Check if a subtypes are set to >= read
				if(result < TYPE_STATE::READ) { // Speedup
					for(auto& i : type->getSubTypes())
						if(this->__transType(i) >= TYPE_STATE::READ) {
							result = std::max(result, TYPE_STATE::READ);
							if(result >= TYPE_STATE::READ)
								break;
						}
				}

				// Check if supertype is set to delete
				if(result < TYPE_STATE::DELETE) // Speedup
					if(this->__isDelete(type))
						result = std::max(result, TYPE_STATE::DELETE);

				// Cache and return
				this->__cache_types[type] = result;
				return result;
			}

			//
			// Cache cleaner
			//
			/**
			 * Clear the caches of a type
			 * @param type the type witches caches should be reseted
			 */
			void __clearTypeCache(const Type* type) const {
				// Clear own
				{
					auto tmp1 = this->__cache_types.find(type);
					auto tmp2 = this->__cache_types_delete.find(type);
					if(tmp1 != this->__cache_types.end())
						this->__cache_types.erase(tmp1);
					if(tmp2 != this->__cache_types_delete.end())
						this->__cache_types_delete.erase(tmp2);
				}

				// Clear supertype
				{
					auto super = getSuper(*type);
					if(super != nullptr)
						this->__clearTypeCache(super);
				}

				// Clear interfaces
				{
					auto& interfaces = getInterfaces(*type);
					for(auto& i : interfaces)
						this->__clearTypeCache(i);
				}
			}

			/**
			 * Clears the cache of a field
			 * IMPORTANT: When jou change a field definition you have to clear the type, in which you changed the field defition on your own!
			 * @param field the field of witch the caches get reseted
			 */
			void __clearFieldCache(const Field* field) const {
				// Clear own
				{
					auto tmp = this->__cache_fields.find(field);
					if(tmp != this->__cache_fields.end())
						this->__cache_fields.erase(tmp);
				}

				// Clear view
				if(field->getMeta().view != nullptr)
					this->__clearFieldCache(field->getMeta().view);

				// Clear referenced types
				for(auto& i : field->getType().types)
					this->__clearTypeCache(i);
			}

			//
			// Helper for updating referenced types in field
			//
			/**
			 * Increase referenced types of a field.
			 * @param field the field witch refernces the types
			 */
			void __incFieldRefernces(const Field* field) {
				// Update referenced types
				for(auto& i : field->getType().types) {
					auto tmp = this->__statesType.find(i);
					if(tmp == this->__statesType.end()) {
						this->__statesType[i] = {0, TYPE_STATE::NO};
						tmp = this->__statesType.find(i);
					}
					std::get<0>(tmp->second) += 1;
				}

				// Update views
				if(field->getMeta().view != nullptr)
					this->__incFieldRefernces(field->getMeta().view);
			}

			/**
			 * Decrement the referenced types of a field.
			 * @param field the field witch references should be decremented
			 */
			void __decFieldReferences(const Field* field) {
				// Update referenced types
				for(auto& i : field->getType().types) {
					auto tmp = this->__statesType.find(i);
					if(tmp == this->__statesType.end())
						throw; // That should never happen!
					std::get<0>(tmp->second) -= 1;
				}

				// Update views
				if(field->getMeta().view != nullptr)
					this->__decFieldReferences(field->getMeta().view);
			}

		public:
			/**
			 * Default constructor of a tool
			 */
			Tool() {}
			/**
			 * Creates a tool
			 * @param name the name of the tool
			 * @param description the description of the name
			 * @param command the command of the tool
			 */
			Tool(std::string name, std::string description, std::string command) : __name(std::move(name)), __description(std::move(description)), __command(std::move(command)) {}

			/**
			 * Returns a reference of the name. Can be used as a setter.
			 * @return reference to the name
			 */
			std::string& getName() { return this->__name; }
			/**
			 * Returns a reference of the description. Can be used as a setter.
			 * @return reference to the description
			 */
			std::string& getDescription() { return this->__description; }
			/**
			 * Returns a reference of the command. Can be used as a setter.
			 * @return reference to the command
			 */
			std::string& getCommand() { return this->__command; }
			/**
			 * Returns a reference of the sates of the fields. Can be used as a setter.
			 * @return reference to the sates of the fields
			 */
			std::unordered_map<const Field*, std::unordered_map<const Type*, FIELD_STATE>>& getStatesFields() { return this->__statesFields; }
			/**
			 * Returns a reference of the states of the types. Can be used as a setter.
			 * @return reference to the states of the types
			 */
			std::unordered_map<const Type*, std::tuple<uint64_t, TYPE_STATE>>& getStatesTypes() { return this->__statesType; }
			/**
			 * Returns the name of the tool.
			 * @return the name
			 */
			const std::string& getName() const { return this->__name; }
			/**
			 * Returns the description of the tool.
			 * @return the description
			 */
			const std::string& getDescription() const { return this->__description; }
			/**
			 * Returns the command of the tool.
			 * @return the command
			 */
			const std::string& getCommand() const{ return this->__command; }
			/**
			 * Returns the states of the fields of the tool.
			 * @return the states of the fields
			 */
			const std::unordered_map<const Field*, std::unordered_map<const Type*, FIELD_STATE>>& getStatesFields() const { return this->__statesFields; }
			/**
			 * Returns the states of the types of the tool.
			 * @return the states of the types
			 */
			const std::unordered_map<const Type*, std::tuple<uint64_t, TYPE_STATE>>& getStatesTypes() const { return this->__statesType; }

			//
			// Get type/field states
			//
			/**
			 * Returns the state of a field
			 * @param field the field to lookup
			 * @param type the type in witch the field was set
			 * @return state of the field. When not set it will return FIELD_STATE::NO
			 */
			FIELD_STATE getFieldSetState(const Field& field, const Type& type) const {
				// Search field
				auto tmp_field = this->__statesFields.find(&field);
				if(tmp_field == this->__statesFields.end())
					return FIELD_STATE::NO;

				// Search type
				auto tmp_type = tmp_field->second.find(&type);
				if(tmp_type == tmp_field->second.end())
					return FIELD_STATE::NO;
				else
					return tmp_type->second;
			}
			/**
			 * Returns the transitive field state.
			 * @param field the field to search
			 * @return the calculated state
			 */
			FIELD_STATE getFieldTransitiveState(const Field& field) const {
				return this->__transField(&field);
			}
			/**
			 * Returns the set state of a type.
			 * @param type the type to use
			 * @return the set state. When not it will return TYPE_STATE::NO
			 */
			TYPE_STATE getTypeSetState(const Type& type) const {
				// Search type
				auto tmp = this->__statesType.find(&type);
				if(tmp == this->__statesType.end())
					return TYPE_STATE::NO;
				else
					return std::get<1>(tmp->second);
			}
			/**
			 * Returns the calculated state of the type.
			 * @param type the type to use
			 * @return state of the type
			 */
			TYPE_STATE getTypeTransitiveState(const Type& type) const {
				return this->__transType(&type);
			}

			//
			// Set field/type state
			//
			/**
			 * Set the state of a field
			 * @param type the type where the field was set
			 * @param field the field that should bet set
			 * @param state the state that should be set
			 */
			void setFieldState(const Type& type, const Field& field, FIELD_STATE state) {
				// Clear caches
				this->__clearTypeCache(&type);
				this->__clearFieldCache(&field);

				// Set state
				FIELD_STATE old = this->getFieldSetState(field, type);
				{
					// Find field
					auto tmp = this->__statesFields.find(&field);
					if(tmp == this->__statesFields.end()) {
						this->__statesFields[&field] = {};
						tmp = this->__statesFields.find(&field);
					}

					// Find type
					auto tmp2 = tmp->second.find(&type);
					if(tmp2 == tmp->second.end()) {
						tmp->second[&type] = FIELD_STATE::NO;
						tmp2 = tmp->second.find(&type);
					}

					// Activate field
					tmp2->second = state;
				}

				// Update type info
				if(old <= FIELD_STATE::UNUSED && state >= FIELD_STATE::READ) {
					// Increase type
					{
						auto tmp = this->__statesType.find(&type);
						if(tmp == this->__statesType.end()) {
							this->__statesType[&type] = {0, TYPE_STATE::NO};
							tmp = this->__statesType.find(&type);
						}
						std::get<0>(tmp->second) += 1;
					}

					// Increase referenced fields
					this->__incFieldRefernces(&field);
				}
				else if(old >= FIELD_STATE::READ && state <= FIELD_STATE::UNUSED) {
					// Decrease type
					{
						auto tmp = this->__statesType.find(&type);
						if(tmp == this->__statesType.end())
							throw; // This should NEVER happen!
						std::get<0>(tmp->second) -= 1;
					}

					// Decrease referenced fields
					this->__decFieldReferences(&field);
				}
			}
			void setTypeState(const Type& type, TYPE_STATE state) {
				// Clear cache
				this->__clearTypeCache(&type);

				// Set state
				{
					auto tmp = this->__statesType.find(&type);
					if(tmp == this->__statesType.end())
						this->__statesType[&type] = {0, state};
					else
						std::get<1>(tmp->second) = state;
				}
			}

			/**
			 * Generates the command line command.
			 * @param spec the filename of the specification
			 * @return commands to run
			 */
			std::string parseCMD(std::string spec) const {
				std::string result = "";
				size_t current = 0;
				while(true) {
					size_t tmp = this->getCommand().find("$CODEGEN", current);
					if(tmp == std::string::npos) {
						result += this->getCommand().substr(current, this->getCommand().size() - current);
						break;
					}
					else {
						result += this->getCommand().substr(current, tmp - current) + "java -jar codegen.jar " + spec;
						current = tmp + 8;
					}
				}
				return result;
			}

			/**
			 * Check if tool is valid.
			 * @param is a valid tool specification
			 */
			bool isValid() const {
				// List types and fields
				std::unordered_set<const Type*> types;
				std::unordered_set<const Field*> fields;
				{
					std::unordered_set<const Type*> todoTypes;
					std::unordered_set<const Field*> todoFields;

					for(auto& i : this->__statesType) // Add all set types
						todoTypes.insert(i.first);
					for(auto& i : this->__statesFields) // Add all types that set a field
						for(auto& j : i.second)
							todoTypes.insert(j.first);

					while(todoTypes.size() > 0 || todoFields.size() > 0) { // Run as long as a type or field has to be processed
						while(todoTypes.size() > 0) { // Add missing types
							const Type* tmp;
							{
								auto tmp2 = todoTypes.begin();
								tmp = *tmp2;
								todoTypes.erase(tmp2);
							}

							if(types.find(tmp) == types.end()) { // Add missing type
								types.insert(tmp);

								{ // Add super type
									const Type* super = getSuper(*tmp);
									if(super != nullptr)
										if(types.find(super) == types.end())
											todoTypes.insert(super);
								}

								{ // Add interfaces
									auto& interfaces = getInterfaces(*tmp);
									for(const Type* i : interfaces)
										if(types.find(i) == types.end())
											todoTypes.insert(i);
								}

								{ // Find fields
									auto& tmpFields = getFields(*tmp);
									for(const Field& i : tmpFields)
										if(fields.find(&i) == fields.end())
											todoFields.insert(&i);
								}
							}
						}

						while(todoFields.size() > 0) { // Add missing fields
							const Field* tmp;
							{
								auto tmp2 = todoFields.begin();
								tmp = *tmp2;
								todoFields.erase(tmp2);
							}

							while(tmp != nullptr) {
								if(fields.find(tmp) == fields.end()) { // Add missing field
									fields.insert(tmp);

									for(const Type* i : tmp->getType().types) // Add types
										if(types.find(i) == types.end())
											todoTypes.insert(i);
								}

								tmp = tmp->getMeta().view;
							}
						}
					}
				}

				// Unset types
				for(auto& i : this->__statesType)
					if(std::get<1>(i.second) == TYPE_STATE::UNUSED)
						if(this->getTypeTransitiveState(*(i.first)) >= TYPE_STATE::READ)
							return false;

				// Unset fields
				for(auto& i : this->__statesFields)
					for(auto& j : i.second)
						if(j.second == FIELD_STATE::UNUSED)
							if(this->getFieldTransitiveState(*(i.first)) >= FIELD_STATE::READ)
								return false;

				// Check coliding types
				for(auto& i : types)
					for(auto& j : i->getCollides())
						if(this->getTypeTransitiveState(*i) >= TYPE_STATE::READ && this->getTypeTransitiveState(*j) >= TYPE_STATE::READ)
							return false;

				// Check coliding fields
				for(auto& i : fields)
					for(auto& j : i->getCollide())
						if(this->getFieldTransitiveState(*i) >= FIELD_STATE::READ && this->getFieldTransitiveState(*j) >= FIELD_STATE::READ)
							return false;

				return true;
			}

			/**
			 * Generates a list of fields that at least readed but not created.
			 * @return fields that the tool needs
			 */
			std::unordered_set<const Field*> requiredFields() const {
				std::unordered_set<const Field*> result;
				for(auto& i : this->__statesFields) {
					const Field* tmp = i.first;
					while(tmp->getMeta().view != nullptr)
						tmp = tmp->getMeta().view;

					auto state = this->getFieldTransitiveState(*tmp);
					if(state >= FIELD_STATE::READ && state != FIELD_STATE::CREATE)
						result.insert(tmp);
				}
				return result;
			}

			/**
			 * Generates a list of fields that create the fields
			 * @return fields that the tool creates
			 */
			std::unordered_set<const Field*> generatedFields() const {
				std::unordered_set<const Field*> result;
				for(auto& i : this->__statesFields) {
					const Field* tmp = i.first;
					while(tmp->getMeta().view != nullptr)
						tmp = tmp->getMeta().view;

					if(this->getFieldTransitiveState(*tmp) == FIELD_STATE::CREATE)
						result.insert(tmp);
				}
				return result;
			}
	};
}
