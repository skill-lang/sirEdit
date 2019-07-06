#include <sirEdit/data/types.hpp>
#include <list>
#include <string>
#include <unordered_map>

namespace {
	using namespace sirEdit::data;
	using namespace std;

	/**
	 * Solves problemes for models
	 */
	class ModelHelper {
		public:

		private:
			unordered_multimap<string, Type*> __types;

		protected:
			/**
			 * Adds a type to update
			 * @param type The type to add
			 * @param name The type name to set
			 */
			void _addType(Type& type) {
				this->__types.insert({type.getName(), &type});
			}

			/**
			 * Updates fields of types and fiedls
			 */
			void _update() {
				// Clear up types
				for(auto& i : this->__types)
					i.second->getSubTypes().clear();

				// Update subtypes
				for(auto& i : this->__types) {
					// Find super
					Type* super = const_cast<Type*>(getSuper(*(i.second)));

					// Add to subtypes list
					if(super != nullptr)
						super->getSubTypes().push_back(i.second);
				}

				// Update interfaces
				for(auto& i : this->__types)
					for(auto& j : getInterfaces(*(i.second)))
						j->getSubTypes().push_back(i.second);

				// TODO: Type inconsistent
				// TODO: Fields
			}
	};

	/**
	 * Model to to test type rules
	 *
	 * g(c) <- h(e)
	 * |
	 * v
	 * a(c) <- b(i)
	 * ^       ^
	 * |       |
	 * c(i) <- d(c) <- e(c)
	 *
	 * i(c) <- f(td)
	 */
	class TypeTestModel1 : ModelHelper {
		public:
			TypeClass a = {"a", "", {}, {}, nullptr};
			TypeInterface b = {"b", "", {}, {}, &a};
			TypeInterface c = {"c", "", {}, {}, &a};
			TypeClass d = {"d", "", {}, {&b, &c}, nullptr};
			TypeClass e = {"e", "", {}, {}, &d};
			TypeTypedef f = {"f", "", &i};
			TypeClass g = {"g", "", {}, {}, &a};
			TypeEnum h = {"h", "", {}, &g};
			TypeClass i = {"i", "", {}, {}, nullptr};

			TypeTestModel1() {
				this->_addType(this->a);
				this->_addType(this->b);
				this->_addType(this->c);
				this->_addType(this->d);
				this->_addType(this->e);
				this->_addType(this->f);
				this->_addType(this->g);
				this->_addType(this->h);
				this->_addType(this->i);

				this->_update();
			}
	};
}
