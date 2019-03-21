#pragma once
#include <memory>
#include <string>
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
			TYPE_STATE __state = TYPE_STATE::NO;

		public:
			Type(std::string name, std::string comment) : __name(std::move(name)), __comment(std::move(comment)) {}
			virtual ~Type() {}

			operator TypeWithFields*();
			operator const TypeWithFields*() const;

			const std::string& getName() const { return this->__name; }
			const std::string& getComment() const { return this->__comment; }
			const std::vector<Type*>& getSubTypes() const { return this->__subTypes; }
			const size_t getID() const { return this->__id; }
			const TYPE_STATE& getState() const { return this->__state; }
			std::string& getName() { return this->__name; }
			std::string& getComment() { return this->__comment; }
			std::vector<Type*>& getSubTypes() { return this->__subTypes; }
			size_t& getID() { return this->__id; }
			TYPE_STATE& getState() { return this->__state; }
	};
	class TypeWithFields : public Type {
		private:
			std::vector<Field> __fields;

		public:
			TypeWithFields(std::string name, std::string comment, std::vector<Field> fields) : Type(std::move(name), std::move(comment)), __fields(std::move(fields)) {}
			TypeWithFields(Type type, std::vector<Field> fields) : Type(std::move(type)), __fields(std::move(fields)) {}

			const std::vector<Field>& getFields() const { return this->__fields; }
			std::vector<Field>& getFields() { return this->__fields; }
	};
	class TypeInterface : public TypeWithFields {
		private:
			std::vector<TypeInterface*> __interfaces;
			Type* __super;

		public:
			TypeInterface(std::string name, std::string comment, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(name), std::move(comment), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}
			TypeInterface(Type type, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(type), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}
			TypeInterface(TypeWithFields fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}

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
			TypeClass(std::string name, std::string comment, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(name), std::move(comment), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}
			TypeClass(Type type, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(type), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}
			TypeClass(TypeWithFields fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}

			const std::vector<TypeInterface*>& getInterfaces() const { return this->__interfaces; }
			const Type* getSuper() const { return this->__super; }
			std::vector<TypeInterface*>& getInterfaces() { return this->__interfaces; }
			Type*& getSuper() { return this->__super; }
	};

	template<class FUNC_BASE, class FUNC_INTERFACE, class FUNC_CLASS>
	inline auto doBaseType(const Type* type, FUNC_BASE funcBase, FUNC_INTERFACE funcInterface, FUNC_CLASS funcClass) {
		if(dynamic_cast<const TypeInterface*>(type) != nullptr)
			return funcInterface();
		else if(dynamic_cast<const TypeClass*>(type) != nullptr)
			return funcClass();
		else
			return funcBase();
	}

	inline Type::operator TypeWithFields*() {
		auto withFields = [this]() -> TypeWithFields* { return dynamic_cast<TypeWithFields*>(this); };
		auto withoutFields = []() -> TypeWithFields* { return nullptr; };
		return doBaseType(this, withoutFields, withFields, withFields);
	}
	inline Type::operator const TypeWithFields*() const {
		auto withFields = [this]() -> const TypeWithFields* { return dynamic_cast<const TypeWithFields*>(this); };
		auto withoutFields = []() -> const TypeWithFields* { return nullptr; };
		return doBaseType(this, withoutFields, withFields, withFields);
	}

	inline const Type* getSuper(const Type& type) {
		auto isBase = []() -> const Type* { return nullptr; };
		auto isInterface = [&type]() -> const Type* { return dynamic_cast<const TypeInterface*>(&type)->getSuper(); };
		auto isClass = [&type]() -> const Type* { return dynamic_cast<const TypeClass*>(&type)->getSuper(); };
		return doBaseType(&type, isBase, isInterface, isClass);
	}
}
