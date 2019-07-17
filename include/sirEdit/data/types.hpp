#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <sirEdit/data/fields.hpp>

namespace sirEdit::data
{
	/**
	 * The state of a type
	 */
	enum struct TYPE_STATE {
		NO,     /// Use the state that match best.
		UNUSED, /// Set force unused
		READ,   /// Allow access to instances of the type
		WRITE,  /// Allow access and add/insert instances of the type
		DELETE  /// Delete every kind of instance from this field. Subtypes will be removed too
	};

	class TypeWithFields;
	/**
	 * Basic type defintion.
	 *
	 * How to init: subTypes, metaType and cllide will be set for you
	 * Set name of the type, comment, hints and restrictions
	 * Use this only for build in types
	 * For classes use TypeClass
	 * For interfaces use TypeInterface
	 * For enums use TypeEnum
	 * For typedefinitions use TypeTypedef
	 * You can use the id to save information that you need internal. It's not required.
	 */
	class Type {
		private:
			std::string __name;                                                       /// The name of the type
			std::string __comment;                                                    /// The comment of the type
			std::vector<Type*> __subTypes;                                            /// List of know subtypes. Will be generated
			size_t __id = -1;                                                         /// Serializer internal id, when required
			std::string __metaTypeName = "<NOT SET>";                                 /// The name of the kind of type
			std::unordered_map<std::string, std::vector<std::string>> __hints;        /// The hints of the type
			std::unordered_map<std::string, std::vector<std::string>> __restrictions; /// The restrictions of the type
			std::vector<const Type*> __collide;                                       /// List of clieded types with same name

		public:
			/**
			 * Copy type without the subsets
			 * @param type the type to copy
			 */
			Type(const Type& type) : __name(type.__name), __comment(type.__comment), __hints(type.__hints), __restrictions(type.__restrictions) {}
			/**
			 * Constructor for a type
			 * @param name The name of the type
			 * @param comment The comment of the type
			 */
			Type(std::string name, std::string comment) : __name(std::move(name)), __comment(std::move(comment)) {}
			/**
			 * Requires virtual deconstructor to deserialize subtypes
			 */
			virtual ~Type() {}

			/**
			 * Support cast to types with fields.
			 * @return type with fields or nullptr when not
			 */
			operator TypeWithFields*();
			/**
			 * Support cast to types with fields.
			 * @return type with fields or nullptr when not
			 */
			operator const TypeWithFields*() const;

			/**
			 * Mostly with have subtypes, so don't do it!
			 */
			Type& operator =(const Type&) = delete;
			/**
			 * Mostly with have subtypes, so don't do it!
			 */
			Type& operator =(Type&& data) = delete;

			/**
			 * Returns the name of the type.
			 * @param the name of the type
			 */
			const std::string& getName() const { return this->__name; }
			/**
			 * Returns the comment of the type.
			 * @param the comment of the type
			 */
			const std::string& getComment() const { return this->__comment; }
			/**
			 * Returns the sub types of the type.
			 * @param the sub types of the type
			 */
			const std::vector<Type*>& getSubTypes() const { return this->__subTypes; }
			/**
			 * Returns the serializer internal id of the type. Can describe all/nothing, so don't change it.
			 * @param the serializer internal id of the type
			 */
			const size_t getID() const { return this->__id; }
			/**
			 * Returns the meta type name of the type.
			 * @param the meta type name of the type
			 */
			const std::string& getMetaTypeName() const { return this->__metaTypeName; }
			/**
			 * Returns the hints of the type.
			 * @param the hints of the type
			 */
			const std::unordered_map<std::string, std::vector<std::string>>& getHints() const { return this->__hints; }
			/**
			 * Returns the restrictions of the type.
			 * @param the restrictions of the type
			 */
			const std::unordered_map<std::string, std::vector<std::string>>& getRestrictions() const { return this->__restrictions; }
			/**
			 * Returns a list collision with other types.
			 * @param a list of collisions to other types
			 */
			const std::vector<const Type*>& getCollides() const { return this->__collide; }
			/**
			 * Returns a reference to the name variable. Can be used as a setter.
			 * @return name of the type
			 */
			std::string& getName() { return this->__name; }
			/**
			 * Returns a reference to the comment variable. Can be used as a setter.
			 * @return comment of the type
			 */
			std::string& getComment() { return this->__comment; }
			/**
			 * Returns a reference to the sub type variable. Can be used as a setter.
			 * @return sub types of the type
			 */
			std::vector<Type*>& getSubTypes() { return this->__subTypes; }
			/**
			 * Returns a reference to the serializer id variable. Can be used as a setter.
			 * @return serializer id of the type
			 */
			size_t& getID() { return this->__id; }
			/**
			 * Returns a reference to the meta type name variable. Can be used as a setter.
			 * @return meta type name of the type
			 */
			std::string& getMetaTypeName() { return this->__metaTypeName; }
			/**
			 * Returns a reference to the hints variable. Can be used as a setter.
			 * @return hints of the type
			 */
			std::unordered_map<std::string, std::vector<std::string>>& getHints() { return this->__hints; }
			/**
			 * Returns a reference to the restrictions variable. Can be used as a setter.
			 * @return restrictions of the type
			 */
			std::unordered_map<std::string, std::vector<std::string>>& getRestrictions() { return this->__restrictions; }
			/**
			 * Returns a reference to the collitions to other types. Can be used as a setter.
			 * @return collitions to other types
			 */
			std::vector<const Type*>& getCollides() { return this->__collide; }
	};
	/**
	 * Type that can have fields.
	 */
	class TypeWithFields : public Type {
		private:
			std::vector<Field> __fields; /// The fields of the type

		public:
			/**
			 * Copy constructor, to copy the fields
			 * @param twf the source of the fields
			 */
			TypeWithFields(const TypeWithFields& twf) : Type(twf), __fields(twf.__fields) {}
			/**
			 * Create a new type with fields
			 * @param name the name of the type
			 * @param comment the comment of the type
			 * @param fields the fields of the type
			 */
			TypeWithFields(std::string name, std::string comment, std::vector<Field> fields) : Type(std::move(name), std::move(comment)), __fields(std::move(fields)) {}
			/**
			 * Creates a new type with fields
			 * @param type the source type to copy
			 * @param fields the fields of the type
			 */
			TypeWithFields(const Type& type, std::vector<Field> fields) : Type(type), __fields(std::move(fields)) {}

			/**
			 * Don't copy it. Fields are dependent on their memory location.
			 */
			TypeWithFields& operator =(const TypeWithFields&) = delete;
			/**
			 * Don't copy it. Fields are dependent on their memory location.
			 */
			TypeWithFields& operator =(TypeWithFields&&) = delete;

			/**
			 * Returns the fields of the type.
			 * @param the fields of the type
			 */
			const std::vector<Field>& getFields() const { return this->__fields; }
			/**
			 * Returns a refernce of the fields. Can be used as setter.
			 * @return refernce to the fields
			 */
			std::vector<Field>& getFields() { return this->__fields; }
	};
	/**
	 * Describs an interface.
	 */
	class TypeInterface : public TypeWithFields {
		private:
			std::vector<TypeInterface*> __interfaces; /// The interfaces of the type
			Type* __super;                            /// The super type

		public:
			/**
			 * Create a new interface.
			 * @param name the name of the type
			 * @param commente the comment of the type
			 * @param fields the fields of the type
			 * @param interfaces the implemented interfaces of the type
			 * @param super the super types of the type
			 */
			TypeInterface(std::string name, std::string comment, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(name), std::move(comment), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "INTERFACE";
			}
			/**
			 * Create a new interface.
			 * @param type the source type to copy
			 * @param fields the fields of the type
			 * @param interfaces the implemented interfaces of the type
			 * @param super the super types of the type
			 */
			TypeInterface(const Type& type, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(type, std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "INTERFACE";
			}
			/**
			 * Create a new interface.
			 * @param fields the source type to copy
			 * @param interfaces the implemented interfaces of the type
			 * @param super the super types of the type
			 */
			TypeInterface(const TypeWithFields& fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(fields), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "INTERFACE";
			}

			/**
			 * Returns a list of interfaces that the type implements
			 * @return implemented interfaces
			 */
			const std::vector<TypeInterface*>& getInterfaces() const { return this->__interfaces; }
			/**
			 * Returns the super type of this type
			 * @return the super type or nullptr
			 */
			const Type* getSuper() const { return this->__super; }
			/**
			 * Returns a reference of a list of interfaces that the type implements. Can be used as setter
			 * @return implemented interfaces
			 */
			std::vector<TypeInterface*>& getInterfaces() { return this->__interfaces; }
			/**
			 * Returns a refernce to super type of this type. Can be used as a setter
			 * @return the super type or nullptr
			 */
			Type*& getSuper() { return this->__super; }
	};
	/**
	 * Describs a class.
	 */
	class TypeClass : public TypeWithFields {
		private:
			std::vector<TypeInterface*> __interfaces; /// The interfaces of the type
			Type* __super;                            /// The super type

		public:
			/**
			 * Create a new class.
			 * @param name the name of the type
			 * @param commente the comment of the type
			 * @param fields the fields of the type
			 * @param interfaces the implemented interfaces of the type
			 * @param super the super types of the type
			 */
			TypeClass(std::string name, std::string comment, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(name), std::move(comment), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "CLASS";
			}
			/**
			 * Create a new class.
			 * @param type the source type to copy
			 * @param fields the fields of the type
			 * @param interfaces the implemented interfaces of the type
			 * @param super the super types of the type
			 */
			TypeClass(const Type& type, std::vector<Field> fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(type), std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "CLASS";
			}
			/**
			 * Create a new class.
			 * @param fields the source type to copy
			 * @param interfaces the implemented interfaces of the type
			 * @param super the super types of the type
			 */
			TypeClass(const TypeWithFields& fields, std::vector<TypeInterface*> interfaces, Type* super) : TypeWithFields(std::move(fields)), __interfaces(std::move(interfaces)), __super(super) {
				this->getMetaTypeName() = "CLASS";
			}

			/**
			 * Returns a list of interfaces that the type implements
			 * @return implemented interfaces
			 */
			const std::vector<TypeInterface*>& getInterfaces() const { return this->__interfaces; }
			/**
			 * Returns the super type of this type
			 * @return the super type or nullptr
			 */
			const Type* getSuper() const { return this->__super; }
			/**
			 * Returns a reference of a list of interfaces that the type implements. Can be used as setter
			 * @return implemented interfaces
			 */
			std::vector<TypeInterface*>& getInterfaces() { return this->__interfaces; }
			/**
			 * Returns a refernce to super type of this type. Can be used as a setter
			 * @return the super type or nullptr
			 */
			Type*& getSuper() { return this->__super; }
	};
	/**
	 * Describs an enum.
	 */
	class TypeEnum : public TypeWithFields {
		private:
			Type* __super; /// The super type

		public:
			/**
			 * Create a new enum.
			 * @param name the name of the type
			 * @param commente the comment of the type
			 * @param fields the fields of the type
			 * @param super the super types of the type
			 */
			TypeEnum(std::string name, std::string comment, std::vector<Field> fields, Type* super) : TypeWithFields(std::move(name), std::move(comment), std::move(fields)), __super(super) {
				this->getMetaTypeName() = "ENUM";
			}
			/**
			 * Create a new enum.
			 * @param type the source type to copy
			 * @param fields the fields of the type
			 * @param super the super types of the type
			 */
			TypeEnum(const Type& type, std::vector<Field> fields, Type* super) : TypeWithFields(std::move(type), std::move(fields)), __super(super) {
				this->getMetaTypeName() = "ENUM";
			}
			/**
			 * Create a new enum.
			 * @param fields the source type to copy
			 * @param super the super types of the type
			 */
			TypeEnum(const TypeWithFields& fields, Type* super) : TypeWithFields(std::move(fields)), __super(super) {
				this->getMetaTypeName() = "ENUM";
			}

			const Type* getSuper() const { return this->__super; }
			Type*& getSuper() { return this->__super; }
	};
	/**
	 * Describs a typedefintion.
	 * Is like a subclass/interface/... of a type without own implementations or fields
	 */
	class TypeTypedef : public Type {
		private:
			const Type* __reference;

		public:
			/**
			 * Create a new typedef.
			 * @param type the source type to copy
			 * @param reference the referenced types of the type
			 */
			TypeTypedef(const Type& type, const Type* reference) : Type(type), __reference(reference) {
				this->getMetaTypeName() = "TYPEDEF";
			}
			/**
			 * Create a new typedef.
			 * @param name the name of the type
			 * @param commente the comment of the type
			 * @param reference the referenced types of the type
			 */
			TypeTypedef(std::string name, std::string comment, const Type* reference) : Type(std::move(name), std::move(comment)), __reference(reference) {
				this->getMetaTypeName() = "TYPEDEF";
			}

			/**
			 * Returns the super type of this type
			 * @return the super type or nullptr
			 */
			const Type* getReference() const { return this->__reference; }
			/**
			 * Returns a refernce to super type of this type. Can be used as a setter
			 * @return the super type or nullptr
			 */
			const Type*& getReference() { return this->__reference; }
	};

	/**
	 * Helper function witch decies if it is a base type, interface, class, enum or a typedef. Don with a dynamic cast.
	 * @tparam FUNC_BASE the type of the function/lambda/... for a base type
	 * @tparam FUNC_INTERFACE the type of the function/lambda/... for a interface
	 * @tparam FUNC_CLASS the type of the function/lambda/... for a class
	 * @tparam FUNC_ENUM the type of the function/lambda/... for a enum
	 * @tparam FUNC_TYPEDEF the type of the function/lambda/... for a typedef
	 * @param type the type to check
	 * @param funcBase the function for a base type
	 * @param funcInterface the function for a interface
	 * @param funcClass the function for a class
	 * @param funcEnum the function for a enum
	 * @param funcTypedef the function for a typedef
	 * @return the return value of the function
	 */
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

	/**
	 * Search the super type of a type
	 * @param type the type to find it's super
	 * @return the super type or nullptr
	 */
	inline const Type* getSuper(const Type& type) {
		auto isBase = []() -> const Type* { return nullptr; };
		auto isInterface = [&type]() -> const Type* { return dynamic_cast<const TypeInterface*>(&type)->getSuper(); };
		auto isClass = [&type]() -> const Type* { return dynamic_cast<const TypeClass*>(&type)->getSuper(); };
		auto isEnum = [&type]() -> const Type* { return dynamic_cast<const TypeEnum*>(&type)->getSuper(); };
		auto isTypedef = [&type]() -> const Type* { return dynamic_cast<const TypeTypedef*>(&type)->getReference(); };
		return doBaseType(&type, isBase, isInterface, isClass, isEnum, isTypedef);
	}

	static const std::vector<TypeInterface*> __getInterfacesEmpty; /// local variable for no interfaces
	/**
	 * Returns a refernce to a list of the interfaces.
	 * @param type The type to find it's interfaces
	 * @return a list with the interfaces
	 */
	inline const std::vector<TypeInterface*>& getInterfaces(const Type& type) {
		auto isBase = []() -> const std::vector<TypeInterface*>& { return __getInterfacesEmpty; };
		auto isInterface = [&type]() -> const std::vector<TypeInterface*>& { return dynamic_cast<const TypeInterface*>(&type)->getInterfaces(); };
		auto isClass = [&type]() -> const std::vector<TypeInterface*>& { return dynamic_cast<const TypeClass*>(&type)->getInterfaces(); };
		return doBaseType(&type, isBase, isInterface, isClass, isBase, isBase);
	}

	static const std::vector<Field> __getFieldsEmpty; /// Empty list of fields
	/**
	 * Returns a reference to a list of the fields.
	 * @param type the source type to find it's fields
	 * @return a list of fields
	 */
	inline const std::vector<Field>& getFields(const Type& type) {
		auto isBase = []() -> const std::vector<Field>& { return __getFieldsEmpty; };
		auto isInterface = [&type]() -> const std::vector<Field>& { return dynamic_cast<const TypeInterface*>(&type)->getFields(); };
		auto isClass = [&type]() -> const std::vector<Field>& { return dynamic_cast<const TypeClass*>(&type)->getFields(); };
		auto isEnum = [&type]() -> const std::vector<Field>& { return dynamic_cast<const TypeEnum*>(&type)->getFields(); };
		return doBaseType(&type, isBase, isInterface, isClass, isEnum, isBase);
	}

	/**
	 * Generates a list of all fields of a type. This means the parents too.
	 * @param type the source type to search
	 * @return list of all fields of the type
	 */
	inline std::unordered_set<const Field*> listAllFields(const Type& type) {
		// Add own fields
		std::unordered_set<const Field*> result;
		for(auto& i : getFields(type))
			result.insert(&i);

		// Add super fields
		{
			auto tmp = getSuper(type);
			if(tmp != nullptr) {
				auto tmp2 = listAllFields(*tmp);
				result.insert(tmp2.begin(), tmp2.end());
			}
		}

		// Add interfaces
		for(auto& i : getInterfaces(type)) {
			auto tmp = listAllFields(*i);
			result.insert(tmp.begin(), tmp.end());
		}

		return result;
	}

	/**
	 * The implementaiton of printing the field declaration.
	 * It's hear, because else no access to the types is possible.
	 * @return declaration of the field as string
	 */
	inline std::string Field::printType() const {
		// Type parse
		std::string result;
		std::string end; // End of the printing
		switch(this->__type.combination) {
			case FieldType::TYPE_COMBINATION::SINGLE_TYPE: // Normal reference
				if(this->__type.types.size() != 1)
					throw; // That sould never happen!
				return this->__type.types[0]->getName();
			case FieldType::TYPE_COMBINATION::DYNAMIC_ARRAY: // A dynamic sized array
				if(this->__type.types.size() != 1)
					throw; // That sould never happen!
				return std::move(this->__type.types[0]->getName() + "[]");
			case FieldType::TYPE_COMBINATION::STATIC_ARRAY: // A static sized array
				if(this->__type.types.size() != 1)
					throw; // That sould never happen!
				return std::move(this->__type.types[0]->getName() + "[" + std::to_string(this->__type.arraySize) + "]");
			case FieldType::TYPE_COMBINATION::LIST: // It's a list
				result = "list<";
				end = ">";
				break;
			case FieldType::TYPE_COMBINATION::SET: // It's a set
				result = "set<";
				end = ">";
				break;
			case FieldType::TYPE_COMBINATION::MAP: // It's a map
				result = "map<";
				end = ">";
				break;
			default:
				throw; // Unkown combination
		}

		// Insert type names
		{
			bool first = true;
			for(auto& i : this->__type.types) {
				if(first)
					result += ", ";
				result += i->getName();
				first = false;
			}
		}
		result += end;

		return result;
	}
}
