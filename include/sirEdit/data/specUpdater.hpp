#pragma once
#include <sirEdit/data/tools.hpp>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <algorithm>

namespace sirEdit::data {
	class SpecModify {
		private:
			mutable std::unordered_map<const Type*, const Type*> oldToNewType;
			mutable std::unordered_map<const Field*, const Field*> oldToNewField;
			std::unordered_set<Field*> fields;
			bool __hasCycle = false;
			std::unordered_map<const Field*, std::vector<Field*>> requiredViews;

			bool compareType(const Type& a, const Type& b) const {
				if(a.getName() != b.getName() ||
					a.getMetaTypeName() != b.getMetaTypeName() ||
					a.getHints() != b.getHints() ||
					a.getRestrictions() != b.getRestrictions())
					return false;
				if((getSuper(a) == nullptr) != (getSuper(b) == nullptr))
					return false;
				if(getSuper(a) != nullptr)
					return this->compareType(*getSuper(a), *getSuper(b));
				return true;
			}

			bool compareField(const Field& a, const Field& b) const {
				if(a.getHints() != b.getHints() ||
					a.getRestrictions() != b.getRestrictions() ||
					a.getMeta().type != b.getMeta().type ||
					a.getMeta().isAuto != b.getMeta().isAuto ||
					a.getMeta().customeLanguage != b.getMeta().customeLanguage ||
					a.getMeta().customeTypename != b.getMeta().customeTypename ||
					a.getMeta().customeOptions != b.getMeta().customeOptions ||
					a.getType().combination != b.getType().combination ||
					a.getType().arraySize != b.getType().arraySize)
						return false;
				if(!this->compareField(*(a.getMeta().view), *(b.getMeta().view)))
					return false;
				if(a.getType().types.size() != b.getType().types.size())
					return false;
				for(size_t i = 0; i < a.getType().types.size(); i++)
					if(!this->compareType(*(a.getType().types[i]), *(b.getType().types[i])))
						return false;
				return true;
			}

			Type* typeCopy(const Type* source, Type* super) {
				// Create new type
				Type* result = doBaseType(source, [source]()-> Type* {
					return new Type(source->getName(), source->getComment());
				}, [source, super]() -> Type* {
					return new TypeInterface(source->getName(), source->getComment(), {}, {}, super);
				}, [source, super]() -> Type* {
					return new TypeClass(source->getName(), source->getComment(), {}, {}, super);
				}, [source, super]() -> Type* {
					return new TypeEnum(source->getName(), source->getComment(), {}, super);
				}, [source, super]() -> Type* {
					return new TypeTypedef(source->getName(), source->getComment(), super);
				});
			}


		public:
			std::vector<Type*> types;

			const Type* findType(const Type& like) const {
				// Try to use cache
				{
					auto tmp = this->oldToNewType.find(&like);
					if(tmp != this->oldToNewType.end())
						return tmp->second;
				}

				for(auto i : this->types) {
					if(this->compareType(like, *i))
						return i;
				}
				return nullptr;
			}
			const Field* findField(const Field& like) const {
				for(auto i : this->fields) {
					if(this->compareField(like, *i))
						return i;
				}
				return nullptr;

				// Update cache and return
				if(result != nullptr)
					this->oldToNewField[&like] = result;
				return result;
			}

			Type* addType(const Type* reference) {
				// Check if allready exists
				auto res = this->findType(*reference);
				if(res !=  nullptr)
					return const_cast<Type*>(res);

				// Add type
				Type* super = const_cast<Type*>(getSuper(*reference));
				if(super != nullptr)
					super = this->addType(super);
				Type* newType = this->typeCopy(reference, super);
				if(dynamic_cast<TypeWithFields*>(newType) != nullptr || dynamic_cast<TypeEnum*>(newType) != nullptr) // Don't add base types!
					this->types.push_back(newType);

				// Add to cache and return
				this->oldToNewType.insert({reference, newType});
				this->oldToNewType.insert({newType, newType}); // Can optimse things
				return newType;
			}

			void meltTypes(std::vector<const Type*> toAdd, std::function<bool(const Type*)> checkType = [](const Type* t) -> bool {return true; }, std::function<bool(const Field*)> checkField = [](const Field* t) -> bool {return true; }) {
				// Add all types
				for(auto i : toAdd)
					if(checkType(i))
						this->addType(i);

				// Update type context
				for(auto i : toAdd) {
					if(!checkType(i))
						continue;
					auto j = this->findType(*i);
					// Add interfaces
					if(dynamic_cast<const TypeClass*>(j) != nullptr || dynamic_cast<const TypeInterface*>(j)) {
						auto& toAdd = const_cast<std::vector<TypeInterface*>&>(getInterfaces(*j));
						for(auto k : getInterfaces(*i)) {
							auto tmp = addType(k);
							if(std::find(toAdd.begin(),toAdd.end(), tmp) == toAdd.end())
								toAdd.push_back(dynamic_cast<TypeInterface*>(tmp));
						}
					}

					// TODO:

			void updateFields() {
				for(auto& i : this->types) {
					oldToNewType[i] = i;
					for(auto& j : getFields(*i))
						this->fields.insert(const_cast<Field*>(&j));
				}
			}

			Tool parseTool(const Tool& tool, const std::vector<const Type*>& toolTypes) {
				// Definitions
				std::unordered_map<std::string, std::vector<Type*>> toolNames; // Find type names
				for(auto& i : this->types) {
					auto tmp = toolNames.find(i->getName());
					if(tmp == toolNames.end())
						toolNames[i->getName()] = {i};
					else
						tmp->second.push_back(i);
				}

				std::function<const Type*(const Type&)> findType = [&](const Type& type) -> const Type* { // Find type
					// Allready now
					{
						auto tmp = this->oldToNewType.find(&type);
						if(tmp != this->oldToNewType.end())
							return tmp->second;
					}

					// Get candidates (name)
					std::unordered_set<const Type*> candidates;
					for(auto& i : this->types)
						if(i->getName() == type.getName())
							candidates.insert(i);
					if(candidates.size() == 1)
						return *(candidates.begin());
					if(candidates.size() == 0)
						return nullptr;

					// Get check if super same
					{
						std::unordered_set<const Type*> tmp;
						for(auto& i : candidates) {
							if((getSuper(*i) == nullptr) != (getSuper(type) == nullptr))
								continue;
							if(findType(type) == findType(*i))
								tmp.insert(i);
						}
						candidates = std::move(tmp);
					}
					if(candidates.size() == 1)
						return *(candidates.begin());
					if(candidates.size() == 0)
						return nullptr;

					// Be exact
					{
						std::unordered_set<const Type*> tmp;
						for(auto& i : candidates) {
							if(i->getComment() != type.getComment())
								continue;
							if(i->getHints() != type.getHints() || i->getRestrictions() != type.getRestrictions())
								continue;
							if(i->getMetaTypeName() == type.getMetaTypeName())
								continue;
							tmp.insert(i);
						}
						candidates = std::move(tmp);
					}
					if(candidates.size() == 0)
						return nullptr;
					else
						return *(candidates.begin());
				};

				std::function<const Field*(const Field&, const Type* type)> findField = [&](const Field& field, const Type* typeTmp) -> const Field* { // Find field
					// Find type
					{
						for(auto& i : toolTypes)
							if(typeTmp == nullptr) {
								for(auto& j : getFields(*i))
									if(&j == &field) {
										typeTmp = i;
										break;
									}
							}
							else
								break;
						if(typeTmp == nullptr)
							return nullptr;
					}
					const Type& type = *typeTmp;

					// Find fields (by name)
					std::unordered_set<const Field*> candidates;
					for(auto& i : getFields(type))
						if(i.getName() == field.getName())
							candidates.insert(&i);
					if(candidates.size() == 1)
						return *(candidates.begin());
					if(candidates.size() == 0)
						return nullptr;

					// Find by type
					{
						std::unordered_set<const Field*> tmp;
						for(auto& i : candidates) {
							if(i->getType().combination != field.getType().combination ||
							   i->getType().arraySize != field.getType().arraySize)
								continue;
							if(i->getMeta().type != field.getMeta().type                       ||
							   i->getMeta().isAuto != field.getMeta().isAuto                   ||
							   i->getMeta().customeLanguage != field.getMeta().customeLanguage ||
							   i->getMeta().customeTypename != field.getMeta().customeTypename ||
							   i->getMeta().customeOptions != field.getMeta().customeOptions   ||
							   i->getMeta().view != field.getMeta().view)
								continue;
							if(field.getType().types.size() != i->getType().types.size())
								continue;
							bool tmp3 = false;
							for(auto& j : field.getType().types) {
								auto tmp2 = findType(*j);
								if(tmp2 == nullptr || tmp2 != findType(*j)) {
									tmp3 = true;
									break;
								}
							}
							if(tmp3)
								continue;
							if((field.getMeta().view == nullptr) != (i->getMeta().view == nullptr))
								continue;
							if(findField(*(i->getMeta().view), nullptr) != findField(*(field.getMeta().view), nullptr))
								continue;
							tmp.insert(i);
						}
						candidates = std::move(tmp);
					}
					if(candidates.size() == 1)
						return *(candidates.begin());
					if(candidates.size() == 0)
						return nullptr;

					// Be exact
					{
						std::unordered_set<const Field*> tmp;
						for(auto& i : candidates) {
							if(i->getComment() != type.getComment())
								continue;
							if(i->getHints() != type.getHints() || i->getRestrictions() != type.getRestrictions())
								continue;
							tmp.insert(i);
						}
						candidates = std::move(tmp);
					}
					if(candidates.size() == 0)
						return nullptr;
					else
						return *(candidates.begin());
				};

				// Update tool
				Tool result = tool;
				result.getStatesFields().clear();
				result.getStatesTypes().clear();

				// Update fields
				for(auto& i : tool.getStatesFields())
					for(auto& j : i.second) {
						if(j.second == FIELD_STATE::NO) // Not important
							continue;
						const Type* type = findType(*(j.first));
						const Field* field = nullptr;
						if(type != nullptr)
							field = findField(*(i.first), type);
						if(type == nullptr || field == nullptr) { // Not found
							if(result.getName().size() == 0 || result.getName()[0] != '!')
								result.getName() = "!" + result.getName();
							result.getDescription() += "\n" + j.first->getName() + "." + i.first->getName() + " was ";
							switch(j.second) {
								case FIELD_STATE::UNUSED:
									result.getDescription() += "UNUSED";
									break;
								case FIELD_STATE::READ:
									result.getDescription() += "READ";
									break;
								case FIELD_STATE::WRITE:
									result.getDescription() += "WRITE";
									break;
								case FIELD_STATE::CREATE:
									result.getDescription() += "CREATE";
									break;
								default:
									throw; // Should never happen!
							}
						}
						else
							result.setFieldState(*type, *field, j.second);
					}

				// Update type
				for(auto& i : tool.getStatesTypes()) {
					if(std::get<1>(i.second) == TYPE_STATE::NO) // Not important
						continue;
					const Type* type = findType(*(i.first));
					if(type == nullptr) {
						if(result.getName().size() == 0 || result.getName()[0] != '!')
							result.getName() = "!" + result.getName();
						result.getDescription() += "\n" + i.first->getName() + " was ";
						switch(std::get<1>(i.second)) {
							case TYPE_STATE::UNUSED:
								result.getDescription() += "UNUSED";
								break;
							case TYPE_STATE::READ:
								result.getDescription() += "READ";
								break;
							case TYPE_STATE::WRITE:
								result.getDescription() += "WRITE";
								break;
							case TYPE_STATE::DELETE:
								result.getDescription() += "DELETE";
								break;
							default:
								throw; // Should never happen!
						}
					}
					else
						result.setTypeState(*type, std::get<1>(i.second));
				}

				// Return tool
				return result;
			}
	};
}

