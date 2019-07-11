#pragma once
#include <algorithm>
#include <string>
#include <tuple>
#include <unordered_map>

#include <sirEdit/data/fields.hpp>
#include <sirEdit/data/types.hpp>

namespace sirEdit::data {
	class Tool {
		private:
			std::string __name;
			std::string __description;
			std::string __command;

			std::unordered_map<const Field*, std::unordered_map<const Type*, FIELD_STATE>> __statesFields;
			std::unordered_map<const Type*, std::tuple<uint64_t, TYPE_STATE>> __statesType;

			//
			// Calc transetive states
			//
			mutable std::unordered_map<const Field*, FIELD_STATE> __cache_fields; /// Cache for fields
			mutable std::unordered_map<const Type*, bool> __cache_types_delete; /// Cache delete state for types
			mutable std::unordered_map<const Type*, TYPE_STATE> __cache_types;  /// Cache state of types

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
			Tool() {}
			Tool(std::string name, std::string description, std::string command) : __name(std::move(name)), __description(std::move(description)), __command(std::move(command)) {}

			std::string& getName() { return this->__name; }
			std::string& getDescription() { return this->__description; }
			std::string& getCommand() { return this->__command; }
			std::unordered_map<const Field*, std::unordered_map<const Type*, FIELD_STATE>>& getStatesFields() { return this->__statesFields; }
			std::unordered_map<const Type*, std::tuple<uint64_t, TYPE_STATE>>& getStatesTypes() { return this->__statesType; }
			const std::string& getName() const { return this->__name; }
			const std::string& getDescription() const { return this->__description; }
			const std::string& getCommand() const{ return this->__command; }
			const std::unordered_map<const Field*, std::unordered_map<const Type*, FIELD_STATE>>& getStatesFields() const { return this->__statesFields; }
			const std::unordered_map<const Type*, std::tuple<uint64_t, TYPE_STATE>>& getStatesTypes() const { return this->__statesType; }

			//
			// Get type/field states
			//
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
			FIELD_STATE getFieldTransitiveState(const Field& field) const {
				return this->__transField(&field);
			}
			TYPE_STATE getTypeSetState(const Type& type) const {
				// Search type
				auto tmp = this->__statesType.find(&type);
				if(tmp == this->__statesType.end())
					return TYPE_STATE::NO;
				else
					return std::get<1>(tmp->second);
			}
			TYPE_STATE getTypeTransitiveState(const Type& type) const {
				return this->__transType(&type);
			}

			//
			// Set field/type state
			//
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
	};
}
