#pragma once
#include <memory>
#include <string>
#include <vector>

#include <sirEdit/data/fields.hpp>

namespace sirEdit::data
{
	class Type {
		private:
			std::string __name;
			std::string __comment;
			std::vector<Type*> __subTypes;
			size_t __id = -1;

		public:
			Type(std::string name, std::string comment) : __name(std::move(name)), __comment(std::move(comment)) {}
			virtual ~Type() {}

			const std::string& getName() const { return this->__name; }
			const std::string& getComment() const { return this->__comment; }
			const std::vector<Type*>& getSubTypes() const { return this->__subTypes; }
			const size_t getID() const { return this->__id; }
			std::string& getName() { return this->__name; }
			std::string& getComment() { return this->__comment; }
			std::vector<Type*>& getSubTypes() { return this->__subTypes; }
			size_t& getID() { return this->__id; }
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
	class TypeClass;
	class TypeInterface : public TypeWithFields {
		private:
			std::vector<TypeInterface*> __interfaces;
			TypeClass* __super;

		public:
			TypeInterface(std::string name, std::string comment, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, TypeClass* super) : TypeWithFields(std::move(name), std::move(comment), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}
			TypeInterface(Type type, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, TypeClass* super) : TypeWithFields(std::move(type), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}
			TypeInterface(TypeWithFields fields, std::vector<TypeInterface*> interfaces, TypeClass* super) : TypeWithFields(std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}

			const std::vector<TypeInterface*>& getInterfaces() const { return this->__interfaces; }
			const TypeClass* getSuper() const { return this->__super; }
			std::vector<TypeInterface*>& getInterfaces() { return this->__interfaces; }
			TypeClass*& getSuper() { return this->__super; }
	};
	class TypeClass : public TypeWithFields {
		private:
			std::vector<TypeInterface*> __interfaces;
			TypeClass* __super;

		public:
			TypeClass(std::string name, std::string comment, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, TypeClass* super) : TypeWithFields(std::move(name), std::move(comment), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}
			TypeClass(Type type, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, TypeClass* super) : TypeWithFields(std::move(type), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}
			TypeClass(TypeWithFields fields, std::vector<TypeInterface*> interfaces, TypeClass* super) : TypeWithFields(std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {}

			const std::vector<TypeInterface*>& getInterfaces() const { return this->__interfaces; }
			const TypeClass* getSuper() const { return this->__super; }
			std::vector<TypeInterface*>& getInterfaces() { return this->__interfaces; }
			TypeClass*& getSuper() { return this->__super; }
	};
}
