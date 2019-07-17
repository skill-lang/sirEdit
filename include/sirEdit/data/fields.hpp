#pragma once
#include <string>
#include <unordered_map>
#include <vector>


namespace sirEdit::data {
	/**
	 * Define the state of a field.
	 */
	enum struct FIELD_STATE {
		NO,     /// No state defined
		UNUSED, /// The field should not be used!
		READ,   /// Read acces to the field
		WRITE,  /// Read and write access to field
		CREATE  /// Read, write access and the tool generates it
	};

	// We need that!
	class Tool;
	class Type;
	class Field;

	/**
	 * The type definition of the field
	 *
	 * How to init as serializer: Set types, combination and arraySize
	 * When it's an enum, let it be how it is (combination = SINGLE_TYPE, types = empty, arraySize = 0)
	 * When not a static array (arraySize = 0)
	 */
	struct FieldType {
		/**
		 * What kind of type it is. Definitions.
		 */
		enum struct TYPE_COMBINATION {
			SINGLE_TYPE,   /// Just a normal type
			DYNAMIC_ARRAY, /// Dynamic sized array
			STATIC_ARRAY,  /// Static sized array (use arraySize to define size)
			LIST,          /// Is a list
			SET,           /// Is a set
			MAP            /// Is a map
		};

		std::vector<Type*> types;     /// The used types in the filed
		TYPE_COMBINATION combination; /// The type of the field
		uint64_t arraySize;           /// The size of a static sized array
	};
	/**
	 * What kind of field it is
	 *
	 * How to init serializer: Set type. Do NOT set viewInverse, it will be done for you
	 * When normal: Let customeLanguage, customeTypename, customeOptions be empty. Set view to nullptr
	 * When custome: Set isAuto to false and view to nullptr
	 * When view: Let customeLanguage, customeTypename, customeOptions be empty. set isAuto to false.
	 * When enum instance: Let customeLanguage, customeTypename, customeOptions be empty. Set isAuto to false and view to nullptr.
	 */
	struct FieldMeta {
		/**
		 * What kind of field it is. Definitions.
		 */
		enum struct META_TYPE {
			NORMAL,       /// A normal type, use FieldType
			CUSTOME,      /// A custome type definition
			VIEW,         /// A view to an other field
			ENUM_INSTANCE /// A enum entry
		};

		META_TYPE type;                                                           /// Define the kind of the field
		bool isAuto = false;                                                      /// Normal field, is it a auto type?
		std::string customeLanguage;                                              /// For customer type define the programming language
		std::string customeTypename;                                              /// For customer type define the type
		std::unordered_map<std::string, std::vector<std::string>> customeOptions; /// The set options of a customer type
		Field* view = nullptr;                                                    /// When it's a view, directing to field
		std::vector<Field*> viewInverse;                                          /// Inverse for calculation of the view field. Reference to all fields that refence
	};
	/**
	 * A Field/Enum instance of a type
	 *
	 * How to init as serializer: Set name, comment, hints, restrictions, type and meta
	 * Look at the specification the from FieldType and FieldMeta.
	 *
	 * collide get calculated for from the abstract serializer.
	 */
	class Field {
		private:
			std::string __name;                                                       /// Name of the field
			std::string __comment;                                                    /// Comment of the type
			FieldType __type;                                                         /// The defined type of the field
			FieldMeta __meta;                                                         /// The defined meta of the field
			std::unordered_map<std::string, std::vector<std::string>> __hints;        /// Hints of the field
			std::unordered_map<std::string, std::vector<std::string>> __restrictions; /// Restrictions of the field
			std::vector<const Field*> __collide;                                      /// Fields the colide

		public:
			/**
			 * Default constructor of a field. std::vector, ... love that!
			 */
			Field() {}
			/**
			 * Genertates a normal field
			 * @param name The name of the field
			 * @param comment The comment of a field
			 * @param type The type of the field
			 */
			Field(std::string name, std::string comment, FieldType type) : __name(std::move(name)), __comment(std::move(comment)), __type(std::move(type)) {
				this->__meta.type = FieldMeta::META_TYPE::NORMAL;
			}

			/**
			 * Returns the name of the field.
			 * @return name of the field
			 */
			const std::string& getName() const { return this->__name; }
			/**
			 * Return the comment of the field
			 * @return comment of the field
			 */
			const std::string& getComment() const { return this->__comment; }
			/**
			 * Returns the type of the field
			 * @return type of the field
			 */
			const FieldType& getType() const { return this->__type; }
			/**
			 * Returns the meta type of the field.
			 * @return meta type of the field
			 */
			const FieldMeta& getMeta() const { return this->__meta; }
			/**
			 * Returns the hints of the field
			 * @return hints of the field
			 */
			const std::unordered_map<std::string, std::vector<std::string>>& getHints() const { return this->__hints; }
			/**
			 * Returns the restrictions of the field
			 * @return restrictions of the field
			 */
			const std::unordered_map<std::string, std::vector<std::string>>& getRestrictions() const { return this->__restrictions; }
			/**
			 * Returns the collision with other fields of this field
			 * @return collisions of the field
			 */
			const std::vector<const Field*> getCollide() const { return this->__collide; }
			/**
			 * Returns reference to the name of the field. Can be used as setter
			 * @return reference to the name
			 */
			std::string& getName() { return this->__name; }
			/**
			 * Returns reference to the comment of the field. Can be used as setter
			 * @return reference to the comment
			 */
			std::string& getComment() { return this->__comment; }
			/**
			 * Returns reference to the type of the field. Can be used as setter
			 * @return reference to the type
			 */
			FieldType& getType() { return this->__type; }
			/**
			 * Returns reference to the meta type of the field. Can be used as setter
			 * @return reference to the meta type
			 */
			FieldMeta& getMeta() { return this->__meta; }
			/**
			 * Returns reference to the hints of the field. Can be used as setter
			 * @return reference to the hints
			 */
			std::unordered_map<std::string, std::vector<std::string>>& getHints() { return this->__hints; }
			/**
			 * Returns reference to the restrictions of the field. Can be used as setter
			 * @return reference to the restrictions
			 */
			std::unordered_map<std::string, std::vector<std::string>>& getRestrictions() { return this->__restrictions; }
			/**
			 * Returns reference to the collisions with other fields of the field. Can be used as setter
			 * @return reference to the collisions
			 */
			std::vector<const Field*> getCollide() { return this->__collide; }

			/**
			 * Prints displayable type of the field.
			 * Implemented in types.hpp
			 * @return type as string
			 */
			std::string printType() const;
	};
}
// Add implementation of printType
#include <sirEdit/data/types.hpp>
