#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <sirEdit/data/fields.hpp>

namespace sirEdit::data
{
	enum struct TYPE_STATE {
		NO,
		UNUSED,
		READ,
		WRITE,
		DELETE
	};

	class TypeWithFields;
	class Type {
		private:
			std::string __name;
			std::string __comment;
			std::vector<Type*> __subTypes;
			size_t __id = -1;
			std::string __metaTypeName = "<NOT SET>";
			std::unordered_map<std::string, std::vector<std::string>> __hints;
			std::unordered_map<std::string, std::vector<std::string>> __restrictions;

		public:
			Type(const Type& type) : __name(type.__name), __comment(type.__comment) {}
			Type(std::string name, std::string comment) : __name(std::move(name)), __comment(std::move(comment)) {}
			virtual ~Type() {}

			operator TypeWithFields*();
			operator const TypeWithFields*() const;

			Type& operator =(const Type&) = delete;
			Type& operator =(Type&& data) = delete;

			const std::string& getName() const { return this->__name; }
			const std::string& getComment() const { return this->__comment; }
			const std::vector<Type*>& getSubTypes() const { return this->__subTypes; }
			const size_t getID() const { return this->__id; }
			const std::string& getMetaTypeName() const { return this->__metaTypeName; }
			const std::unordered_map<std::string, std::vector<std::string>>& getHints() const { return this->__hints; }
			const std::unordered_map<std::string, std::vector<std::string>>& getRestrictions() const { return this->__restrictions; }
			std::string& getName() { return this->__name; }
			std::string& getComment() { return this->__comment; }
			std::vector<Type*>& getSubTypes() { return this->__subTypes; }
			size_t& getID() { return this->__id; }
			std::string& getMetaTypeName() { return this->__metaTypeName; }
			std::unordered_map<std::string, std::vector<std::string>>& getHints() { return this->__hints; }
			std::unordered_map<std::string, std::vector<std::string>>& getRestrictions() { return this->__restrictions; }
	};
	class TypeWithFields : public Type {
		private:
			std::vector<Field> __fields;

		public:
			TypeWithFields(const TypeWithFields& twf) : Type(twf), __fields(twf.__fields) {}
			TypeWithFields(std::string name, std::string comment, std::vector<Field> fields) : Type(std::move(name), std::move(comment)), __fields(std::move(fields)) {}
			TypeWithFields(const Type& type, std::vector<Field> fields) : Type(type), __fields(std::move(fields)) {}

			TypeWithFields& operator =(const TypeWithFields&) = delete;
			TypeWithFields& operator =(TypeWithFields&&) = delete;

			const std::vector<Field>& getFields() const { return this->__fields; }
			std::vector<Field>& getFields() { return this->__fields; }
	};
	class TypeInterface : public TypeWithFields {
		private:
			std::vector<TypeInterface*> __interfaces;
			Type* __super;

		public:
			TypeInterface(std::string name, std::string comment, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(name), std::move(comment), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "INTERFACE";
			}
			TypeInterface(const Type& type, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(type, std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "INTERFACE";
			}
			TypeInterface(const TypeWithFields& fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(fields), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "INTERFACE";
			}

			const std::vector<TypeInterface*>& getInterfaces() const { return this->__interfaces; }
			const Type* getSuper() const { return this->__super; }
			std::vector<TypeInterface*>& getInterfaces() { return this->__interfaces; }
			Type*& getSuper() { return this->__super; }
	};
	class TypeClass : public TypeWithFields {
		private:
			std::vector<TypeInterface*> __interfaces;
			Type* __super;

		public:
			TypeClass(std::string name, std::string comment, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(name), std::move(comment), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "CLASS";
			}
			TypeClass(const Type& type, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(type), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "CLASS";
			}
			TypeClass(const TypeWithFields& fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "CLASS";
			}

			const std::vector<TypeInterface*>& getInterfaces() const { return this->__interfaces; }
			const Type* getSuper() const { return this->__super; }
			std::vector<TypeInterface*>& getInterfaces() { return this->__interfaces; }
			Type*& getSuper() { return this->__super; }
	};
	class TypeEnum : public TypeWithFields {
		private:
			Type* __super;

		public:
			TypeEnum(std::string name, std::string comment, std::vector<Field> fields, Type* super) : TypeWithFields(std::move(name), std::move(comment), std::move(fields)), __super(super) {
				this->getMetaTypeName() = "ENUM";
			}
			TypeEnum(const Type& type, std::vector<Field> fields, Type* super) : TypeWithFields(std::move(type), std::move(fields)), __super(super) {
				this->getMetaTypeName() = "ENUM";
			}
			TypeEnum(const TypeWithFields& fields, Type* super) : TypeWithFields(std::move(fields)), __super(super) {
				this->getMetaTypeName() = "ENUM";
			}

			const Type* getSuper() const { return this->__super; }
			Type*& getSuper() { return this->__super; }
	};
	class TypeTypedef : public Type {
		private:
			const Type* __reference;

		public:
			TypeTypedef(const Type& type, const Type* reference) : Type(type), __reference(reference) {
				this->getMetaTypeName() = "TYPEDEF";
			}
			TypeTypedef(std::string name, std::string comment, const Type* reference) : Type(std::move(name), std::move(comment)), __reference(reference) {
				this->getMetaTypeName() = "TYPEDEF";
			}

			const Type* getReference() const { return this->__reference; }
			const Type*& getReference() { return this->__reference; }
	};

	template<class FUNC_BASE, class FUNC_INTERFACE, class FUNC_CLASS, class FUNC_ENUM, class FUNC_TYPEDEF>
	inline decltype((*static_cast<FUNC_BASE*>(nullptr))()) doBaseType(const Type* type, FUNC_BASE funcBase, FUNC_INTERFACE funcInterface, FUNC_CLASS funcClass, FUNC_ENUM funcEnum, FUNC_TYPEDEF funcTypedef) {
		if(dynamic_cast<const TypeInterface*>(type) != nullptr)
			return funcInterface();
		else if(dynamic_cast<const TypeClass*>(type) != nullptr)
			return funcClass();
		else if(dynamic_cast<const TypeEnum*>(type) != nullptr)
			return funcEnum();
		else if(dynamic_cast<const TypeTypedef*>(type) != nullptr)
			return funcTypedef();
		else
			return funcBase();
	}

	inline Type::operator TypeWithFields*() {
		auto withFields = [this]() -> TypeWithFields* { return dynamic_cast<TypeWithFields*>(this); };
		auto withoutFields = []() -> TypeWithFields* { return nullptr; };
		return doBaseType(this, withoutFields, withFields, withFields, withFields, withoutFields);
	}
	inline Type::operator const TypeWithFields*() const {
		auto withFields = [this]() -> const TypeWithFields* { return dynamic_cast<const TypeWithFields*>(this); };
		auto withoutFields = []() -> const TypeWithFields* { return nullptr; };
		return doBaseType(this, withoutFields, withFields, withFields, withFields, withoutFields);
	}

	inline const Type* getSuper(const Type& type) {
		auto isBase = []() -> const Type* { return nullptr; };
		auto isInterface = [&type]() -> const Type* { return dynamic_cast<const TypeInterface*>(&type)->getSuper(); };
		auto isClass = [&type]() -> const Type* { return dynamic_cast<const TypeClass*>(&type)->getSuper(); };
		auto isEnum = [&type]() -> const Type* { return dynamic_cast<const TypeEnum*>(&type)->getSuper(); };
		auto isTypedef = [&type]() -> const Type* { return dynamic_cast<const TypeTypedef*>(&type)->getReference(); };
		return doBaseType(&type, isBase, isInterface, isClass, isEnum, isTypedef);
	}

	static const std::vector<TypeInterface*> __getInterfacesEmpty;
	inline const std::vector<TypeInterface*>& getInterfaces(const Type& type) {
		auto isBase = []() -> const std::vector<TypeInterface*>& { return __getInterfacesEmpty; };
		auto isInterface = [&type]() -> const std::vector<TypeInterface*>& { return dynamic_cast<const TypeInterface*>(&type)->getInterfaces(); };
		auto isClass = [&type]() -> const std::vector<TypeInterface*>& { return dynamic_cast<const TypeClass*>(&type)->getInterfaces(); };
		return doBaseType(&type, isBase, isInterface, isClass, isBase, isBase);
	}

	static const std::vector<Field> __getFieldsEmpty;
	inline const std::vector<Field>& getFields(const Type& type) {
		auto isBase = []() -> const std::vector<Field>& { return __getFieldsEmpty; };
		auto isInterface = [&type]() -> const std::vector<Field>& { return dynamic_cast<const TypeInterface*>(&type)->getFields(); };
		auto isClass = [&type]() -> const std::vector<Field>& { return dynamic_cast<const TypeClass*>(&type)->getFields(); };
		auto isEnum = [&type]() -> const std::vector<Field>& { return dynamic_cast<const TypeEnum*>(&type)->getFields(); };
		return doBaseType(&type, isBase, isInterface, isClass, isEnum, isBase);
	}

	inline std::string Field::printType() const {
		// Type parse
		std::string result;
		switch(this->__type.combination) {
			case FieldType::TYPE_COMBINATION::SINGLE_TYPE:
				if(this->__type.types.size() != 1)
					throw; // That sould never happen!
				return this->__type.types[0]->getName();
			case FieldType::TYPE_COMBINATION::DYNAMIC_ARRAY:
				if(this->__type.types.size() != 1)
					throw; // That sould never happen!
				return std::move(this->__type.types[0]->getName() + "[]");
			case FieldType::TYPE_COMBINATION::STATIC_ARRAY:
				if(this->__type.types.size() != 1)
					throw; // That sould never happen!
				return std::move(this->__type.types[0]->getName() + "[" + std::to_string(this->__type.arraySize) + "]");
			case FieldType::TYPE_COMBINATION::LIST:
				result = "list";
				break;
			case FieldType::TYPE_COMBINATION::SET:
				result = "set";
				break;
			case FieldType::TYPE_COMBINATION::MAP:
				result = "map";
				break;
			default:
				throw; // Unkown combination
		}

		// Insert template
		result += "<";
		{
			bool first = true;
			for(auto& i : this->__type.types) {
				if(first)
					result += ", ";
				result += i->getName();
				first = false;
			}
		}
		result += ">";

		return result;
	}
}
