#include <sirEdit/data/tools.hpp>
#include <sirEdit/data/serialize.hpp>
#include <gtkmm.h>
#include <unordered_set>

#include <sirEdit/main.hpp>

using namespace sirEdit::data;

class ToolModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<Glib::ustring> data_sort_name;
		Gtk::TreeModelColumn<bool> data_used;
		Gtk::TreeModelColumn<bool> data_active;
		Gtk::TreeModelColumn<Tool*> data_tool;

		ToolModel() {
			this->add(data_used);
			this->add(data_active);
			this->add(data_name);
			this->add(data_sort_name);
			this->add(data_tool);
		}
};
static ToolModel toolModel;

class TypeModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<Glib::ustring> data_sort_name;
		Gtk::TreeModelColumn<bool> data_used;
		Gtk::TreeModelColumn<bool> data_active;
		Gtk::TreeModelColumn<Type*> data_type;

		TypeModel() {
			this->add(data_used);
			this->add(data_active);
			this->add(data_name);
			this->add(data_sort_name);
			this->add(data_type);
		}
};
static TypeModel typeModel;

class FieldModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<Glib::ustring> data_sort_name;
		Gtk::TreeModelColumn<bool> data_used;
		Gtk::TreeModelColumn<bool> data_active;
		Gtk::TreeModelColumn<Field*> data_field;
		Gtk::TreeModelColumn<bool> data_isUsable;

		FieldModel() {
			this->add(data_used);
			this->add(data_active);
			this->add(data_name);
			this->add(data_sort_name);
			this->add(data_field);
			this->add(data_isUsable);
		}
};
static FieldModel fieldModel;


class Overview : public Gtk::VBox {
	private:
		Transactions& transactions;

		// Stack
		Gtk::Stack stack;
		Gtk::StackSwitcher stack_switcher;

		// Type overview
		Gtk::HPaned tool_paned;
		Gtk::HPaned type_paned;
		Gtk::HPaned field_paned;

		Gtk::ToggleButton hide_inactive_tool = Gtk::ToggleButton("Hide inactive");
		Gtk::ToggleButton hide_unused_tool = Gtk::ToggleButton("Hide unused");
		Gtk::ToggleButton hide_inactive_type = Gtk::ToggleButton("Hide inactive");
		Gtk::ToggleButton hide_unused_type = Gtk::ToggleButton("Hide unused");
		Gtk::ToggleButton hide_inactive_field = Gtk::ToggleButton("Hide inactive");
		Gtk::ToggleButton hide_unused_field = Gtk::ToggleButton("Hide unused");

		Gtk::ScrolledWindow tool_scroll;
		Gtk::TreeView tool_view;
		Glib::RefPtr<Gtk::TreeStore> tool_store;
		Gtk::ScrolledWindow type_scroll;
		Gtk::TreeView type_view;
		Glib::RefPtr<Gtk::TreeStore> type_store;
		Gtk::ScrolledWindow field_scroll;
		Gtk::TreeView field_view;
		Glib::RefPtr<Gtk::TreeStore> field_store;

		Gtk::VBox sidebar;

		//
		// Render sidebar
		//

		inline void clearSidebar() {
			auto tmp = this->sidebar.get_children();
			for(auto& i : tmp)
				this->sidebar.remove(*i);
		}
		inline Gtk::Label* createLabel(const std::string& text) {
			Gtk::Label* result = Gtk::manage(new Gtk::Label(text));
			result->set_xalign(0);
			result->set_line_wrap(true);
			return result;
		};

		inline void renderToolSidebar(const Tool& tool) {
			// Create base structure
			Gtk::VBox* typesSet;
			Gtk::VBox* fieldsSet;
			{
				this->clearSidebar();
				this->sidebar.pack_start(*(createLabel(std::string("Name: ") + tool.getName())), false, false);
				this->sidebar.pack_start(*(createLabel(std::string("Description:\n") + tool.getDescription())), false, false);
				this->sidebar.pack_start(*(createLabel(std::string("Command-Line:\n") + tool.getCommand())), false, false);
				typesSet = Gtk::manage(new Gtk::VBox());
				this->sidebar.pack_start(*typesSet, false, false);
				fieldsSet = Gtk::manage(new Gtk::VBox());
				this->sidebar.pack_start(*fieldsSet, false, false);
			}

			// Add types
			{ // TODO: Sort
				bool first = true;
				for(auto& i : tool.getStatesTypes())
					if(std::get<1>(i.second) >= TYPE_STATE::UNUSED) {
						if(first) {
							first = false;
							typesSet->pack_start(*(createLabel(std::string(" "))), false, false);
							typesSet->pack_start(*(createLabel(std::string("Types:"))), false, false);
						}
						const char* state;
						switch(std::get<1>(i.second)) {
							case TYPE_STATE::UNUSED:
								state = "UNUSED";
								break;
							case TYPE_STATE::READ:
								state = "READ";
								break;
							case TYPE_STATE::WRITE:
								state = "WRITE";
								break;
							case TYPE_STATE::DELETE:
								state = "DELETE";
								break;
							default:
								throw; // That should never happen!
						}
						typesSet->pack_start(*(createLabel(i.first->getName() + " : " + state)), false, false);
					}
			}

			// TODO: Fileds

			this->sidebar.show_all();
		}

		template<class SOURCE>
		inline void renderSidebarHints(const SOURCE& source, Gtk::VBox& target) {
			// Hints
			{
				bool first = true;
				for(auto& i : source.getHints()) {
					if(first) {
						first = false;
						target.pack_start(*(createLabel(std::string(" "))), false, false);
						target.pack_start(*(createLabel(std::string("Hints:"))), false, false);
					}

					std::string tmp = i.first;
					if(i.second.size() > 0) {
						tmp += "(";
						bool first2 = true;
						for(auto& j : i.second) {
							if(first2)
								first2 = false;
							else
								tmp += ", ";
							tmp += j;
						}
						tmp += ")";
					}
					target.pack_start(*(createLabel(tmp)), false, false);
				}
			}

			// Restrictions
			{
				bool first = true;
				for(auto& i : source.getRestrictions()) {
					if(first) {
						first = false;
						target.pack_start(*(createLabel(std::string(" "))), false, false);
						target.pack_start(*(createLabel(std::string("Restrictions:"))), false, false);
					}

					std::string tmp = i.first;
					if(i.second.size() > 0) {
						tmp += "(";
						bool first2 = true;
						for(auto& j : i.second) {
							if(first2)
								first2 = false;
							else
								tmp += ", ";
							tmp += j;
						}
						tmp += ")";
					}
					target.pack_start(*(createLabel(tmp)), false, false);
				}
			}
		}
		// TODO: Show type
		inline void renderFieldSidebar(const Field& field) {
			// Base structure
			Gtk::VBox* hints;
			Gtk::VBox* types;
			{
				this->clearSidebar();
				this->sidebar.pack_start(*(createLabel(std::string("Name: ") + field.getName())), false, false);
				this->sidebar.pack_start(*(createLabel(std::string("Description:\n") + field.getComment())), false, false);
				this->sidebar.pack_start(*(createLabel(std::string("Type: ") + field.printType())), false, false);
				hints = Gtk::manage(new Gtk::VBox());
				this->sidebar.pack_start(*hints, false, false);
				types = Gtk::manage(new Gtk::VBox());
				this->sidebar.pack_start(*types, false, false);
			}

			// Generate hints
			renderSidebarHints(field, *hints);

			// Used in tools
			{ // TODO: Sort
				bool first = true;
				for(auto& i : this->transactions.getData().getTools()) {
					FIELD_STATE fs = i->getFieldTransitiveState(field);
					if(fs > FIELD_STATE::UNUSED) {
						if(first) {
							first = false;
							types->pack_start(*(createLabel(std::string(" "))), false, false);
							types->pack_start(*(createLabel(std::string("Types:"))), false, false);
						}
						const char* state;
						switch(fs) {
							case FIELD_STATE::READ:
								state = "READ";
								break;
							case FIELD_STATE::WRITE:
								state = "WRITE";
								break;
							case FIELD_STATE::CREATE:
								state = "CREATE";
								break;
							default:
								throw; // That should never happen!
						}
						types->pack_start(*(createLabel(i->getName() + " : " + state)), false, false);
					}
				}
			}

			// Finalize
			this->sidebar.show_all();
		}

		//
		// Cache
		//
		std::unordered_set<const Type*> __cache_used_type;
		std::unordered_set<const Type*> __cache_selected_type;
		std::unordered_set<const Tool*> __cache_selected_tools;
		std::unordered_set<const Tool*> __cache_used_tools;
		std::unordered_set<const Field*> __cache_used_field;
		std::unordered_set<const Field*> __cache_selected_fields;
		const Type* __current_type = nullptr;

		std::unordered_set<const Type*> getActiveTypes() {
			std::unordered_set<const Type*> result;
			for(auto& i : this->type_view.get_selection()->get_selected_rows()) {
				const Type* tmp = (*(this->type_store->get_iter(i)))[typeModel.data_type];
				if(tmp != nullptr)
					result.insert(tmp);
			}
			return std::move(result);
		}
		std::unordered_set<const Field*> getActiveFields() {
			std::unordered_set<const Field*> result;
			for(auto& i : this->field_view.get_selection()->get_selected_rows()) {
				const Field* tmp = (*(this->field_store->get_iter(i)))[fieldModel.data_field];
				if(tmp != nullptr)
					result.insert(tmp);
			}
			return std::move(result);
		}
		void genCacheTools() {
			this->__cache_selected_tools = {};
			for(auto& i : this->tool_view.get_selection()->get_selected_rows()) {
				const Tool* tmp = (*(this->tool_store->get_iter(i)))[toolModel.data_tool];
				if(tmp != nullptr)
					this->__cache_selected_tools.insert(tmp);
			}
		}
		void genCacheTypes() {
			this->__cache_selected_type = std::move(this->getActiveTypes());
		}
		void genCacheFields() {
			this->__cache_selected_fields = std::move(this->getActiveFields());
		}

		//
		// Helpers
		//
		bool blockChange = false;

		bool isToolActive(const Tool* tool) {
			// Check marked tools
			if(this->__cache_selected_tools.size() != 0) {
				bool found = false;
				for(auto& i : this->__cache_selected_tools) {
					if(i == tool) {
						found = true;
						break;
					}
				}
				if(!found)
					return false;
			}

			// Check type
			if(this->__cache_selected_type.size() != 0) {
				bool found = false;
				for(auto& i : this->__cache_selected_type) {
					if(tool->getTypeTransitiveState(*i) >= TYPE_STATE::READ) {
						found = true;
						break;
					}
				}
				if(!found)
					return false;
			}

			// Check field
			if(this->__cache_selected_fields.size() != 0) {
				bool found = false;
				for(auto& i : this->__cache_selected_fields) {
					auto tmp = tool->getStatesFields().find(i);
					if(tmp != tool->getStatesFields().end()) {
						for(auto& j : tmp->second)
							if(j.second >= FIELD_STATE::READ) {
								found = true;
								break;
							}
						if(found)
							break;
					}
				}
				if(!found)
					return false;
			}
			return true;
		}
		bool isTypeActive(const Type* type) {
			// Check selected types
			if(this->__cache_selected_type.size() != 0) {
				bool found = false;
				for(auto& i : this->__cache_selected_type) {
					if(i == type) {
						found = true;
						break;
					}
				}
				if(!found)
					return false;
			}

			// Check tools
			if(this->__cache_selected_tools.size() != 0) {
				bool found = false;
				for(auto& i : this->__cache_selected_tools) {
					if(i->getTypeTransitiveState(*type) >= TYPE_STATE::READ) {
						found = true;
						break;
					}
				}
				if(!found)
					return false;
			}

			// Check field
			if(this->__cache_selected_fields.size() != 0) {
				bool found = false;
				for(auto& i : this->__cache_selected_fields) {
					for(auto& j : this->transactions.getData().getTools())
						if(j->getFieldSetState(*i, *type) >= FIELD_STATE::READ) {
							found = true;
							break;
						}
					if(found)
						break;
				}
				if(!found)
					return false;
			}
			return true;
		}
		bool isFieldActive(const Field* field) {
			// Check tools
			if(this->__cache_selected_tools.size() > 0) {
				bool found = false;
				for(auto& i : this->__cache_selected_tools) {
					for(auto& j : i->getStatesFields()) { // TODO: Optimize
						if(j.first == field)
							for(auto& k : j.second)
								if(k.second >= FIELD_STATE::READ) {
									found = true;
									break;
								}
						if(found)
							break;
					}
					if(found)
						break;
				}
				if(!found)
					return false;
			}

			// Check types
			if(this->__cache_selected_type.size() > 0) {
				bool found = false;
				for(auto& i : this->transactions.getData().getTools()) {
					for(auto& j : i->getStatesFields()) { // TODO: Optimize
						if(j.first == field)
							for(auto& k : this->__cache_selected_type) {
								auto tmp = j.second.find(k);
								if(tmp != j.second.end())
									if(tmp->second >= FIELD_STATE::READ) {
										found = true;
										break;
									}
							}
						if(found)
							break;
					}
					if(found)
						break;
				}
				if(!found)
					return false;
			}

			// Check fields
			if(this->__cache_selected_fields.size() > 0) {
				bool found = false;
				for(auto& i : this->__cache_selected_fields)
					if(i == field) {
						found = true;
						break;
					}
				if(!found)
					return false;
			}
			return true;
		}

		void updateToolData() {
			this->tool_view.get_selection()->unselect_all();
			this->tool_store->clear();
			this->tool_store->set_sort_column(toolModel.data_sort_name, Gtk::SortType::SORT_ASCENDING);

			// Empty selection
			{
				auto tmp = *(this->tool_store->append());
				tmp[toolModel.data_name] = "-- NO SELECTION --";
				tmp[toolModel.data_sort_name] = "a";
				tmp[toolModel.data_active] = false;
				tmp[toolModel.data_used] = false;
				tmp[toolModel.data_tool] = nullptr;
			}

			// Add Tools
			for(auto& i : this->transactions.getData().getTools()) {
				// Checks
				if(this->hide_unused_tool.property_active().get_value() && this->__cache_used_tools.find(i) == this->__cache_used_tools.end())
					continue;
				if(this->hide_inactive_tool.property_active().get_value() && !this->isToolActive(i))
					continue;

				// Insert
				auto tmp = *(this->tool_store->append());
				tmp[toolModel.data_name] = i->getName();
				tmp[toolModel.data_sort_name] = "b_" + i->getName();
				tmp[toolModel.data_active] = this->isToolActive(i);
				tmp[toolModel.data_used] = this->__cache_used_tools.find(i) != this->__cache_used_tools.end();
				tmp[toolModel.data_tool] = const_cast<Tool*>(i);
			}

			// Select tools
			for(auto& i : this->tool_store->children())
				if(this->__cache_selected_tools.find((*i)[toolModel.data_tool]) != this->__cache_selected_tools.end())
					this->tool_view.get_selection()->select(i);
		}

		template<bool FIRST>
		void __genTypeItem(Gtk::TreeStore& tree, Gtk::TreeStore::Row& row, const Type* type) {
			// Checks
			if(this->hide_unused_type.property_active().get_value() && this->__cache_used_type.find(type) == this->__cache_used_type.end()) {
				for(auto& i : type->getSubTypes())
					if(getSuper(*i) == type)
						this->__genTypeItem<FIRST>(tree, row, i);
				return;
			}
			if(this->hide_inactive_type.property_active().get_value() && !this->isTypeActive(type)) {
				for(auto& i : type->getSubTypes())
					if(getSuper(*i) == type)
						this->__genTypeItem<FIRST>(tree, row, i);
				return;
			}

			// Insert
			Gtk::TreeStore::Row own;
			Gtk::TreeStore::iterator ownIter;
			{
				if constexpr(FIRST)
					ownIter = tree.append();
				else
					ownIter = tree.append(row.children());
			}
			own = *ownIter;
			own[typeModel.data_name] = type->getName() + " : " + type->getMetaTypeName();
			own[typeModel.data_sort_name] = "b_" + type->getName();
			own[typeModel.data_active] = this->isTypeActive(type);
			own[typeModel.data_used] = this->__cache_used_type.find(type) != this->__cache_used_type.end();
			own[typeModel.data_type] = const_cast<Type*>(type);
			for(auto& i : type->getSubTypes())
				if(getSuper(*i) == type)
					this->__genTypeItem<false>(tree, own, i);
		}
		void selectTypes(Gtk::TreeStore::iterator iter) {
			// Check entry
			if(this->__cache_selected_type.find((*iter)[typeModel.data_type]) != this->__cache_selected_type.end())
				this->type_view.get_selection()->select(iter);

			// Update children
			for(auto& i : iter->children())
				this->selectTypes(i);
		}
		void updateTypeData() {
			// Get scroll bar
			double x = this->type_scroll.get_hadjustment()->get_value();
			double y = this->type_scroll.get_vadjustment()->get_value();

			// Update entries
			this->type_view.get_selection()->unselect_all();
			this->type_store->clear(); // TODO: Just clear when required.
			this->type_store->set_sort_column(typeModel.data_sort_name, Gtk::SortType::SORT_ASCENDING);

			//Add empty
			{
				auto tmp = *(this->type_store->append());
				tmp[typeModel.data_name] = "-- NO SELECTION --";
				tmp[typeModel.data_sort_name] = "a";
				tmp[typeModel.data_active] = false;
				tmp[typeModel.data_used] = false;
				tmp[typeModel.data_type] = nullptr;
			}
			{
				Gtk::TreeStore::Row tmp;
				for(auto& i : this->transactions.getData().getBaseTypes())
					this->__genTypeItem<true>(*(this->type_store.get()), tmp, i);
				this->type_view.expand_all();
				for(auto& i : this->type_store->children())
					this->selectTypes(i);
			}

			// Set scroll bar
			this->type_scroll.get_hadjustment()->set_value(x);
			this->type_scroll.get_vadjustment()->set_value(y);
		}

		Gtk::TreeStore::Path __genFieldData(const Type* type, bool required, Gtk::TreeStore::Path* insertInto) {
			// Find local fields to show
			auto fields = getFields(*type);

			// Generate class iter
			const Type* parent = getSuper(*type);
			Gtk::TreeStore::iterator classIter;
			if(insertInto != nullptr)
				classIter = this->field_store->append(this->field_store->get_iter(*insertInto)->children()); // TODO: Check if is empty
			else if(parent != nullptr) {
				Gtk::TreeStore::Path parentPath = this->__genFieldData(parent, required | fields.size() > 0, nullptr);
				if(!required & fields.size() == 0)
					return {};
				classIter = this->field_store->append(this->field_store->get_iter(parentPath)->children());
			}
			else {
				if(!required & fields.size() == 0)
					return {};
				classIter = this->field_store->append();
			}

			// Set class info
			{
				auto tmp = *classIter;
				tmp[fieldModel.data_isUsable] = false;
				tmp[fieldModel.data_active] = false;
				tmp[fieldModel.data_used] = false;
				tmp[fieldModel.data_field] = nullptr;
				tmp[fieldModel.data_name] = type->getName() + " : " +  type->getMetaTypeName();
				tmp[fieldModel.data_sort_name] = "b_" + type->getName();
			}

			// Add interfaces
			for(auto& i : getInterfaces(*type)) {
				Gtk::TreeStore::Path tmp = this->field_store->get_path(classIter);
				this->__genFieldData(i, false, &tmp);
			}

			// Show data
			for(auto& i : fields) {
				auto tmpIter = this->field_store->append(classIter->children());
				auto tmp = *tmpIter;
				tmp[fieldModel.data_active] = this->isFieldActive(&i);
				tmp[fieldModel.data_isUsable] = true;
				tmp[fieldModel.data_field] = &i;
				tmp[fieldModel.data_name] = i.getName() + " : " + i.printType();
				tmp[fieldModel.data_used] = this->__cache_used_field.find(&i) != this->__cache_used_field.end();
				tmp[fieldModel.data_sort_name] = "c_" + i.getName();
			}

			return this->field_store->get_path(classIter);
		}
		void selectFields(Gtk::TreeStore::iterator iter) {
			// Update this
			{
				const Field* field = (*iter)[fieldModel.data_field];
				if(this->__cache_selected_fields.find(field) != this->__cache_selected_fields.end())
					this->field_view.get_selection()->select(iter);
			}

			// Update children
			for(auto& i : iter->children())
				this->selectFields(i);
		}
		void updateFieldData() {
			// Get scrollbar
			double x = this->field_scroll.get_hadjustment()->get_value();
			double y = this->field_scroll.get_vadjustment()->get_value();

			// TODO: Check filter
			this->field_view.get_selection()->unselect_all();
			this->field_store->clear();
			this->field_store->set_sort_column(fieldModel.data_sort_name, Gtk::SortType::SORT_ASCENDING);
			{
				// Add fiedls
				if(this->__current_type != nullptr) {
					// Empty slot
					{
						auto tmp = *(this->field_store->append());
						tmp[fieldModel.data_active] = false;
						tmp[fieldModel.data_isUsable] = true;
						tmp[fieldModel.data_field] = nullptr;
						tmp[fieldModel.data_name] = "-- NO SELECTION --";
						tmp[fieldModel.data_used] = false;
						tmp[fieldModel.data_sort_name] = "a";
					}

					this->__genFieldData(this->__current_type, false, nullptr);
				}
				this->field_view.expand_all();

				// Select
				for(auto& i : this->field_store->children())
					this->selectFields(i);
			}

			// Set scrollbar
			this->field_scroll.get_hadjustment()->set_value(x);
			this->field_scroll.get_vadjustment()->set_value(y);
		}

		//
		// Generators
		//
		template<bool USED_CHECK = false, class MODEL, class TOGGLE_CALLBACK>
		static void genTreeView(Gtk::TreeView& tree_view, MODEL& tree_model, const TOGGLE_CALLBACK& toggle_callback) {
			auto tmp_used = Gtk::manage(new Gtk::CellRendererToggle());
			tree_view.append_column("used", *tmp_used);
			tree_view.get_column(0)->add_attribute(tmp_used->property_active(), tree_model.data_used);
			tmp_used->signal_toggled().connect(toggle_callback);
			if constexpr(USED_CHECK)
				tree_view.get_column(0)->add_attribute(tmp_used->property_activatable(), tree_model.data_isUsable);
			auto tmp_active = Gtk::manage(new Gtk::CellRendererToggle());
			tmp_active->set_activatable(false);
			tree_view.append_column("active", *tmp_active);
			tree_view.get_column(1)->add_attribute(tmp_active->property_active(), tree_model.data_active);
			tree_view.append_column("name", tree_model.data_name);
			tree_view.set_search_column(2);
			tree_view.set_expander_column(*(tree_view.get_column(2)));
			tree_view.expand_all();
			tree_view.get_selection()->set_mode(Gtk::SelectionMode::SELECTION_MULTIPLE);
		}

		//
		// Events
		//
		void toggle_tool_used(const Glib::ustring& index) {
			auto tmp = *(this->tool_store->get_iter(index));
			auto tmpInter = this->__cache_used_tools.find(static_cast<const Tool*>(tmp[toolModel.data_tool]));
			if(tmpInter == this->__cache_used_tools.end()) {
				this->__cache_used_tools.insert(static_cast<const Tool*>(tmp[toolModel.data_tool]));
				tmp[toolModel.data_used] = true;
			}
			else {
				this->__cache_used_tools.erase(tmpInter);
				tmp[toolModel.data_used] = false;
			}
			this->update_all();
		}
		void toggle_type_used(const Glib::ustring& index) {
			auto tmp = *(this->type_store->get_iter(index));
			auto tmpIter = this->__cache_used_type.find(static_cast<const Type*>(tmp[typeModel.data_type]));
			if(tmpIter == this->__cache_used_type.end()) {
				this->__cache_used_type.insert(static_cast<const Type*>(tmp[typeModel.data_type]));
				tmp[typeModel.data_used] = true;
			}
			else {
				this->__cache_used_type.erase(tmpIter);
				tmp[typeModel.data_used] = false;
			}
			this->update_all();
		}
		void toggle_field_used(const Glib::ustring& index) {
			auto tmp = *(this->field_store->get_iter(index));
			auto tmpIter = this->__cache_used_field.find(static_cast<const Field*>(tmp[fieldModel.data_field]));
			if(tmpIter == this->__cache_used_field.end()) {
				this->__cache_used_field.insert(static_cast<const Field*>(tmp[fieldModel.data_field]));
				tmp[fieldModel.data_used] = true;
			}
			else {
				this->__cache_used_field.erase(tmpIter);
				tmp[fieldModel.data_used] = false;
			}
			this->update_all();
		}

		void update_all() {
			if(blockChange)
				return;
			blockChange = true;

			// Update views
			this->genCacheTools();
			this->genCacheTypes();
			this->genCacheFields();
			this->updateToolData();
			this->updateTypeData();
			this->updateFieldData();

			blockChange = false;
		}

	public:
		Overview(Transactions& transactions) : Gtk::VBox(), transactions(transactions) {
			// Init Stack
			this->stack.add(this->tool_paned, "Tool Overview", "Tool Overview");
			this->stack.add(*Gtk::manage(new Gtk::HBox()), "Tool Relations", "Tool Relations"); // TODO: tool relations
			this->stack_switcher.set_stack(this->stack);
			{
				Gtk::Alignment* tmp = Gtk::manage(new Gtk::Alignment());
				tmp->set(0.5, 0.5, 0, 0);
				tmp->add(this->stack_switcher);
				this->pack_start(*tmp, false, true);
			}
			this->pack_start(this->stack, true, true);

			// Init layout
			this->tool_paned.pack2(this->type_paned);
			{
				Gtk::VBox* tmp = Gtk::manage(new Gtk::VBox());
				tmp->pack_start(this->hide_inactive_tool, false, true);
				this->hide_inactive_tool.signal_toggled().connect([this]() -> void {
					this->update_all();
				});
				tmp->pack_start(this->hide_unused_tool, false, true);
				this->hide_unused_tool.signal_toggled().connect([this]() -> void {
					this->update_all();
				});
				this->tool_scroll.add(this->tool_view);
				tmp->pack_start(this->tool_scroll, true, true);
				this->tool_paned.pack1(*tmp);
			}
			this->type_paned.pack2(this->field_paned);
			{
				Gtk::VBox* tmp = Gtk::manage(new Gtk::VBox());
				tmp->pack_start(this->hide_inactive_type, false, true);
				this->hide_inactive_type.signal_toggled().connect([this]() -> void {
					this->update_all();
				});
				tmp->pack_start(this->hide_unused_type, false, true);
				this->hide_unused_type.signal_toggled().connect([this]() -> void {
					this->update_all();
				});
				this->type_scroll.add(this->type_view);
				tmp->pack_start(this->type_scroll, true, true);
				this->type_paned.pack1(*tmp);
			}
			{
				Gtk::VBox* tmp = Gtk::manage(new Gtk::VBox());
				tmp->pack_start(this->hide_inactive_field, false, true);
				this->hide_inactive_field.signal_toggled().connect([this]() -> void {
					this->update_all();
				});
				tmp->pack_start(this->hide_unused_field, false, true);
				this->hide_unused_field.signal_toggled().connect([this]() -> void {
					this->update_all();
				});
				this->field_scroll.add(this->field_view);
				tmp->pack_start(this->field_scroll, true, true);
				this->field_paned.pack1(*tmp);
			}
			{
				Gtk::ScrolledWindow* tmp = Gtk::manage(new Gtk::ScrolledWindow());
				tmp->add(this->sidebar);
				this->field_paned.pack2(*tmp);
			}

			// Trees
			this->tool_store = Gtk::TreeStore::create(toolModel);
			this->tool_view.set_model(this->tool_store);
			genTreeView(this->tool_view, toolModel, [this](Glib::ustring index) -> void {
				this->toggle_tool_used(index);
			});
			this->tool_view.get_selection()->signal_changed().connect([this]() -> void {
				this->update_all();
			});
			this->tool_view.signal_row_activated().connect([this](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) -> void {
				const Tool* tool = (*(this->tool_store->get_iter(path)))[toolModel.data_tool];
				if(tool != nullptr)
					this->renderToolSidebar(*tool);
			});

			this->type_store = Gtk::TreeStore::create(typeModel);
			this->type_view.set_model(this->type_store);
			genTreeView(this->type_view, typeModel, [this](Glib::ustring index) -> void {
				this->toggle_type_used(index);
			});
			this->type_view.get_selection()->signal_changed().connect([this]() -> void {
				this->update_all();
			});
			this->type_view.signal_row_activated().connect([this](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) -> void {
				const Type* type = (*(this->type_store->get_iter(path)))[typeModel.data_type];
				if(type != nullptr) {
					this->__current_type = type;
					this->update_all();
				}
			});

			this->field_store = Gtk::TreeStore::create(fieldModel);
			this->field_view.set_model(this->field_store);
			genTreeView<true>(this->field_view, fieldModel, [this](Glib::ustring index) -> void {
				this->toggle_field_used(index);
			});
			this->field_view.get_selection()->signal_changed().connect([this]() -> void {
				this->update_all();
			});
			this->field_view.signal_row_activated().connect([this](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) -> void {
				const Field* field = (*(this->field_store->get_iter(path)))[fieldModel.data_field];
				if(field != nullptr)
					this->renderFieldSidebar(*field);
			});

			// Content
			this->update_all();

			// Change notification
			this->transactions.addChangeCallback([this]() -> void {
				this->update_all();
			});
		}
};

namespace sirEdit::gui {
	Gtk::Widget* createOverview(Transactions& transactions) {
		return new Overview(transactions);
	}
}
