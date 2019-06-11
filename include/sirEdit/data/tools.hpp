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
			// Cache for deep search of type
			//
			mutable std::unordered_map<const Type*, bool> __cacheTransitiveType;
			std::unordered_map<const Type*, uint64_t> __setTypeFields;
			bool __hasReadSubtype(const Type& type) const {
				// Test cache
				{
					auto tmp = this->__cacheTransitiveType.find(&type);
					if(tmp != this->__cacheTransitiveType.end())
						return tmp->second;
				}

				// Test self
				{
					auto tmp = this->__statesType.find(&type);
					if((tmp != this->__statesType.end()) && (std::get<0>(tmp->second) > 0 || std::get<1>(tmp->second) >= TYPE_STATE::READ)) {
						this->__cacheTransitiveType[&type] = true;
						return true;
					}
				}

				// Test subtypes
				for(auto& i : type.getSubTypes())
					if(this->__hasReadSubtype(*i)) {
						this->__cacheTransitiveType[&type] = true;
						return true;
					}

				// Test fields
				{
					auto tmp = this->__setTypeFields.find(&type);
					if(tmp != this->__setTypeFields.end() && tmp->second > 0) {
						this->__cacheTransitiveType[&type] = true;
						return true;
					}
				}

				// Not found
				this->__cacheTransitiveType[&type] = false;
				return false;
			}

			inline void __setField(const Field& field) {
				for(auto& i : field.getType().types) {
					// Set state
					auto tmp = this->__setTypeFields.find(i);
					if(tmp == this->__setTypeFields.end())
						this->__setTypeFields[i] = 1;
					else
						this->__setTypeFields[i]++;

					// Clean cache
					this->__cleanCacheType(*i);
				}
			}
			inline void __unsetField(const Field& field) {
				for(auto& i : field.getType().types) {
					// Unset
					this->__setTypeFields[i]--;

					// Clean cache
					this->__cleanCacheType(*i);
				}
			}

			//
			// Helper
			//
			void __cleanCacheType(const Type& type) {
				// Clear deep search cache
				{
					const Type* current = &type;
					while(current != nullptr) {
						 // Remove current
						auto tmp = this->__cacheTransitiveType.find(current);
						if(tmp != this->__cacheTransitiveType.end())
							this->__cacheTransitiveType.erase(tmp);

						// Do interfaces
						for(auto& i : getInterfaces(*current))
							this->__cleanCacheType(*i);
						current = getSuper(*current);
					}
				}
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
				// Search field
				auto tmp = this->__statesFields.find(&field);
				if(tmp == this->__statesFields.end())
					return FIELD_STATE::UNUSED;

				// Compute max
				FIELD_STATE result = FIELD_STATE::UNUSED;
				for(auto& i : tmp->second)
					result = std::max(result, i.second);
				return result;
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
				// Is at least read?
				if(!this->__hasReadSubtype(type))
					return TYPE_STATE::NO;

				// Get current state
				TYPE_STATE result = TYPE_STATE::READ;
				{
					// Check if field gets created
					for(auto& i : getFields(type))
						if(this->getFieldTransitiveState(i) == FIELD_STATE::CREATE)
							result = std::max(TYPE_STATE::WRITE, result);

					// Get set state
					auto tmp = this->__statesType.find(&type);
					if(tmp != this->__statesType.end())
						result = std::max(std::get<1>(tmp->second), result);
				}

				// Check for parents delete
				{
					const Type* current = getSuper(type);
					while(current != nullptr) {
						auto tmp = this->__statesType.find(current);
						if((tmp != this->__statesType.end()) && (std::get<1>(tmp->second) == TYPE_STATE::DELETE))
							return TYPE_STATE::DELETE;
						current = getSuper(*current);
					}
				}

				return result;
			}

			//
			// Set field/type state
			//
			void setFieldState(const Type& type, const Field& field, FIELD_STATE state) {
				// Clear caches
				this->__cleanCacheType(type);

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
					if(state >= FIELD_STATE::READ && tmp2->second <= FIELD_STATE::UNUSED)
						this->__setField(field);
					else if(state <= FIELD_STATE::UNUSED && tmp2->second >= FIELD_STATE::READ)
						this->__unsetField(field);

					tmp->second[&type] = state;
				}

				// Update type info
				if(old <= FIELD_STATE::UNUSED & state >= FIELD_STATE::READ) {
					// Increase type
					auto tmp = this->__statesType.find(&type);
					if(tmp == this->__statesType.end()) {
						this->__statesType[&type] = {0, TYPE_STATE::NO};
						tmp = this->__statesType.find(&type);
					}
					std::get<0>(tmp->second)++;
				}
				else if(old >= FIELD_STATE::READ & state <= FIELD_STATE::UNUSED) {
					// Decrease type
					auto tmp = this->__statesType.find(&type);
					if(tmp == this->__statesType.end())
						throw; // This should NEVER happen!
					std::get<0>(tmp->second)--;
				}
			}
			void setTypeState(const Type& type, TYPE_STATE state) {
				// Clear cache
				this->__cleanCacheType(type);

				// Set state
				{
					auto tmp = this->__statesType.find(&type);
					if(tmp == this->__statesType.end())
						this->__statesType[&type] = {0, state};
					else
						std::get<1>(tmp->second) = state;
				}
			}
	};
}
