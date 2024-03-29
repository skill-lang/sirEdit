#pragma once
#include <sirEdit/data/tools.hpp>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <algorithm>

namespace sirEdit::data {
	/**
	 * The helper class can copies the types and fields of a specification and can meld a specification to the current result.
	 * This class allso supports the import of tools, to find the corosponding type and field.
	 * Also it can be used to generte a tool specification with help of the import filters.
	 */
	class SpecModify {
		private:
			mutable std::unordered_map<const Type*, const Type*> oldToNewType;    /// Is a cache witch allows to look up old converted types to new ones.
			mutable std::unordered_map<const Field*, const Field*> oldToNewField; /// Is a cache for the source of an old field to the equal new one.
			std::unordered_set<Field*> fields;                                    /// A list of all added fields
			bool __hasCycle = false;                                              /// Has interfaces has a cycle
			std::unordered_map<const Field*, std::vector<Field*>> requiredViews;  /// Fields that need to be updated (view conent)

			/**
			 * Compares two types. Bouth types have to have the same hints, restrictions, name, kind and compareable super classes.
			 * @param a first type to compare
			 * @param b second type to compare
			 * @return a equal enought for b (and the other way round too).
			 */
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

			/**
			 * Compares two fields. Bouth types have to have the same hints, restrictions, name, type, and meta data.
			 * If it's a view field then the view has to be check too.
			 * @param a first field to compare
			 * @param b second field to compare
			 * @return a equal enought for b (and the other way round too).
			 */
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

			/**
			 * Copies the type from source and set super as it's super type.
			 * @param source the source to copy
			 * @param super the super type to set
			 * @return the new copied type.
			 */
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

				// Set hints and restriciton and return
				result->getHints() = source->getHints();
				result->getRestrictions() = source->getRestrictions();
				return result;
			}

			/**
			 * Copy a field to an other
			 * @param source the source field to copy
			 * @param target the target to copy
			 */
			void fieldCopyTo(const Field& source, Field& target) {
				// Copy basedata
				target.getType() = source.getType();
				target.getMeta() = source.getMeta();
				target.getName() = source.getName();
				target.getComment() = source.getComment();
				target.getHints() = source.getHints();
				target.getRestrictions() = source.getRestrictions();
				this->oldToNewField[&target] = &source;

				// Update types
				for(auto& i : target.getType().types)
					i = this->addType(i);

				// Update view
				if(source.getMeta().view != nullptr) {
					auto tmp = this->oldToNewField.find(&source);
					if(tmp == this->oldToNewField.end()) {
						auto tmp2 = this->requiredViews.find(&target);
						if(tmp2 == this->requiredViews.end())
							tmp2->second.push_back(&target);
						else
							this->requiredViews[&source] = {&target};
					}
					else
						const_cast<const Field*&>(source.getMeta().view) = tmp->second;
				}

				// Update remote views
				{
					auto tmp = this->requiredViews.find(&source);
					if(tmp != this->requiredViews.end()) {
						for(auto& i : tmp->second)
							i->getMeta().view = &target;
						tmp->second.clear();
					}
				}
			}

			/**
			 * Checks if interface has a cycle.
			 * @param typeList list of allrady added interfaces
			 * @param interface the current interface
			 * @return if this interface builds a cycle
			 */
			bool calcCycle(std::unordered_set<Type*>& typeList, TypeInterface* interface) {
				if(typeList.find(interface) != typeList.end())
					return true;
				typeList.insert(interface);
				for(auto& i : interface->getInterfaces())
					if(this->calcCycle(typeList, i))
						return true;
				typeList.erase(interface);
				return false;
			}

		public:
			std::vector<Type*> types; /// The copied new types

			/**
			 * Search a type, with is equal enought.
			 * @param like the type with sould be used as key for the search.
			 * @return the founded type or null
			 */
			const Type* findType(const Type& like) const {
				// Try to use cache
				{
					auto tmp = this->oldToNewType.find(&like);
					if(tmp != this->oldToNewType.end())
						return tmp->second;
				}

				// Search type
				const Type* result = nullptr;
				for(auto i : this->types) {
					if(this->compareType(like, *i)) {
						result = i;
						break;
					}
				}

				// Cache an return
				if(result != nullptr)
					this->oldToNewType[&like] = result;
				return result;
			}

			/**
			 * Search a the new filed witch is simular to like.
			 * @param like the field that should be equal enought to be searched.
			 * @return the founded new field or null.
			 */
			const Field* findField(const Field& like) const {
				// Try to use the cache
				{
					auto tmp = this->oldToNewField.find(&like);
					if(tmp != this->oldToNewField.end())
						return tmp->second;
				}

				// Cache entry not found
				const Field* result = nullptr;
				for(auto i : this->fields) {
					if(this->compareField(like, *i)) {
						result = i;
						break;
					}
				}

				// Update cache and return
				if(result != nullptr)
					this->oldToNewField[&like] = result;
				return result;
			}

			/**
			 * Copies a type as long as it doesn't exists allready.
			 * @param reference the source type to copy
			 * @return the new type
			 */
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

			/**
			 * Melt types from an other specification into this one. The types an fields get copied
			 * @param toAdd the types to add
			 * @param checkType function to contole import of specific types. Usefull to generate toolspecification.
			 * @param checkField function to controle impport of a specific fields. Usefull to generatoe tool specificiations.
			 */
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

					// TODO: Fields
				}

				// Cycle check
				for(auto& i : this->types)
					if(dynamic_cast<TypeInterface*>(i) != nullptr) {
						std::unordered_set<Type*> cycle;
						this->__hasCycle |= this->calcCycle(cycle, dynamic_cast<TypeInterface*>(i));
					}
			}

			/**
			 * Result if interfaces build a cycle
			 * @return true when interfaces build a cycle
			 */
			bool hasCycle() const { return this->__hasCycle; }

			/**
			 * When types are set outside add fields.
			 */
			void updateFields() {
				for(auto& i : this->types) {
					oldToNewType[i] = i;
					for(auto& j : getFields(*i))
						this->fields.insert(const_cast<Field*>(&j));
				}
			}

			/**
			 * Import a tool
			 * @param tool the tool to import
			 * @param toolTypes a list of types that the tool uses
			 * @return the new tool
			 */
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

			/**
			 * Export types to sir. Types are after this lost!
			 * @return path to the sir file.
			 */
			std::string dropToSIR();

			/**
			 * Export types to skill file. Types are after this lost!
			 * @return path to the skill file
			 */
			std::string dropToSKilL();
	};
}

