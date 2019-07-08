#pragma once
#include <sirEdit/data/tools.hpp>
#include <unordered_map>
#include <functional>
#include <algorithm>

namespace sirEdit::data {
	class SpecModify {
		private:
			std::unordered_map<const Type*, const Type*> oldToNewType;
			std::unordered_map<const Field*, const Field*> oldToNewField;
			std::vector<Field*> fields;

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
				return doBaseType(source, [source]()-> Type* {
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
			}
			Type* addType(const Type* reference) {
				auto res = this->findType(*reference);
				if(res !=  nullptr)
					return const_cast<Type*>(res);

				// Add type
				Type* super = const_cast<Type*>(getSuper(*reference));
				if(super != nullptr)
					super = this->addType(super);
				Type* newType = this->typeCopy(reference, super);
				this->types.push_back(newType);
				return newType;
			}

			void meldTypes(std::vector<const Type*> toAdd, std::function<bool(const Type*)> checkType = [](const Type* t) -> bool {return true; }, std::function<bool(const Field*)> checkField = [](const Field* t) -> bool {return true; }) {
				for(auto i : toAdd)
					if(checkType(i))
						this->addType(i);
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
				}

				// TODO: check circles
			}
	};
}

