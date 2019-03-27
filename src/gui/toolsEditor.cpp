#include "mainWindow.hpp"
#include <gtkmm.h>
#include <unordered_map>

#include <iostream>
#include <sirEdit/main.hpp>

using namespace sirEdit;
using namespace sirEdit::data;
using namespace std;

//
// Model for type  list
//
class TypeListModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<size_t> data_id;
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<bool> data_status_r;
		Gtk::TreeModelColumn<bool> data_status_r_transitive;
		Gtk::TreeModelColumn<bool> data_status_w;
		Gtk::TreeModelColumn<bool> data_status_w_transitive;
		Gtk::TreeModelColumn<bool> data_status_d;
		Gtk::TreeModelColumn<bool> data_status_d_transitive;
		Gtk::TreeModelColumn<bool> data_status_u;
		Gtk::TreeModelColumn<bool> data_status_u_transitive;
		Gtk::TreeModelColumn<bool> data_status_no;

		TypeListModel() {
			this->add(data_id);
			this->add(data_status_no);
			this->add(data_status_u);
			this->add(data_status_u_transitive);
			this->add(data_status_r);
			this->add(data_status_r_transitive);
			this->add(data_status_w);
			this->add(data_status_w_transitive);
			this->add(data_status_d);
			this->add(data_status_d_transitive);
			this->add(data_name);
		}
};
static TypeListModel typeListModel;
//static Glib::RefPtr<Gtk::TreeStore> typeListData;

//
// Field tree
//
class FieldListModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<Glib::ustring> data_sort_name;
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<bool> data_status_active;
		Gtk::TreeModelColumn<bool> data_status_r;
		Gtk::TreeModelColumn<bool> data_status_r_transitive;
		Gtk::TreeModelColumn<bool> data_status_w;
		Gtk::TreeModelColumn<bool> data_status_w_transitive;
		Gtk::TreeModelColumn<bool> data_status_e;
		Gtk::TreeModelColumn<bool> data_status_e_transitive;
		Gtk::TreeModelColumn<bool> data_status_u;
		Gtk::TreeModelColumn<bool> data_status_u_transitive;
		Gtk::TreeModelColumn<bool> data_status_no;
		Gtk::TreeModelColumn<bool> data_status_no_transitive;

		FieldListModel() {
			this->add(data_sort_name);
			this->add(data_name);
			this->add(data_status_active);
			this->add(data_status_no);
			this->add(data_status_no_transitive);
			this->add(data_status_u);
			this->add(data_status_u_transitive);
			this->add(data_status_r);
			this->add(data_status_r_transitive);
			this->add(data_status_w);
			this->add(data_status_w_transitive);
			this->add(data_status_e);
			this->add(data_status_e_transitive);
		}
};
static FieldListModel fieldListModel;
//static Glib::RefPtr<Gtk::TreeStore> fieldListData;

/**
 * Manage a complete tab.
 */
class Tab : public Gtk::HPaned
{
	private:
		Tool& tool;
		Type* currentType;

		Glib::RefPtr<Gtk::TreeStore> typeListData;
		Glib::RefPtr<Gtk::TreeStore> fieldListData;

		Gtk::ScrolledWindow scrollable_left;
		Gtk::TreeView view_left;
		Gtk::ScrolledWindow scrollable_right;
		Gtk::TreeView view_right;

		std::unordered_map<string, sirEdit::data::Field*> field_lookup;

		//
		// General functions
		//
		/**
		 * Set the state of an fields row entry.
		 * @param row The row witch should be set.
		 * @param all The state witch is set transitive from other tabs.
		 * @param set The current set state from this type.
		 */
		void fieldUpdate(Gtk::TreeStore::Row row, FIELD_STATE all, FIELD_STATE set) {
			// Unset
			row[fieldListModel.data_status_no] = false;
			row[fieldListModel.data_status_no_transitive] = false;
			row[fieldListModel.data_status_u] = false;
			row[fieldListModel.data_status_u_transitive] = false;
			row[fieldListModel.data_status_r] = false;
			row[fieldListModel.data_status_r_transitive] = false;
			row[fieldListModel.data_status_w] = false;
			row[fieldListModel.data_status_w_transitive] = false;
			row[fieldListModel.data_status_e] = false;
			row[fieldListModel.data_status_e_transitive] = false;

			// Set
			row[fieldListModel.data_status_no] = set == FIELD_STATE::NO;
			row[fieldListModel.data_status_u] = set == FIELD_STATE::UNUSED;
			row[fieldListModel.data_status_u_transitive] = all <= FIELD_STATE::UNUSED & set != FIELD_STATE::UNUSED;
			row[fieldListModel.data_status_r] = set == FIELD_STATE::READ;
			row[fieldListModel.data_status_r_transitive] = all >= FIELD_STATE::READ & set != FIELD_STATE::READ;
			row[fieldListModel.data_status_w] = set == FIELD_STATE::WRITE;
			row[fieldListModel.data_status_w_transitive] = all >= FIELD_STATE::WRITE & set != FIELD_STATE::WRITE;
			row[fieldListModel.data_status_e] = set == FIELD_STATE::CREATE;
			row[fieldListModel.data_status_e_transitive] = all >= FIELD_STATE::CREATE & set != FIELD_STATE::CREATE;
		}

		/**
		 * Search the local state of the field of the current type.
		 * @param field The field to search.
		 * @return Current state of a field in this tool for the current state.
		 */
		FIELD_STATE getFieldState(const Field& field) {
			// Find tool
			auto tmp_tool = field.getStates().find(&(this->tool));
			if(tmp_tool == field.getStates().end())
				return FIELD_STATE::NO;

			// Search status
			auto tmp_state = tmp_tool->second.find(this->currentType);
			if(tmp_state == tmp_tool->second.end())
				return FIELD_STATE::NO;
			return tmp_state->second;
		}

		//
		// Generators
		//
		void buildTreeSubRows(Gtk::TreeStore::Row& row, const Type& super) {
			for(auto& i : super.getSubTypes()) {
				Gtk::TreeStore::Row tmp = *(typeListData->append(row.children()));
				tmp[typeListModel.data_id] = i->getID();
				tmp[typeListModel.data_name] = i->getName();
				buildTreeSubRows(tmp, *i);
			}
		}

		void buildFieldsLayer(const Type& type, unordered_map<const Type*, Gtk::TreeModel::Row>& tmp_types) {
			// Check if exists double
			if(tmp_types.find(&type) != tmp_types.end())
				return;

			// Call super
			auto super = sirEdit::data::getSuper(*type);
			if(super != nullptr)
				this->buildFieldsLayer(*super, tmp_types);

			// Create type row
			Gtk::TreeStore::Row typeRow;
			if(super != nullptr)
				typeRow = *(fieldListData->append(tmp_types[super]->children()));
			else
				typeRow = *(fieldListData->append());
			tmp_types[type] = typeRow;
			typeRow[fieldListModel.data_name] = type.getName();
			typeRow[fieldListModel.data_sort_name] = std::string("a_") + type.getName();
			typeRow[fieldListModel.data_status_active] = false;

			// Insert fields
			{
				const sirEdit::data::TypeWithFields* fields = *type;
				if(fields != nullptr)
					for(auto& i : fields->getFields()) {
						Gtk::TreeStore::Row tmp = *(fieldListData->append(typeRow.children()));
						tmp[fieldListModel.data_name] = i.getName();
						tmp[fieldListModel.data_sort_name] = std::string("b_") + i.getName();
						tmp[fieldListModel.data_status_active] = true;
						this->field_lookup[i.getName()] = const_cast<Field*>(&i);
						this->fieldUpdate(tmp, i.getToolType(this->tool), getFieldState(i));
					}
			}
		}

		//
		// Events
		//
		void event_type_clicked(const Gtk::TreeModel::Path& path) {
			Gtk::TreeModel::iterator iter = this->typeListData->get_iter(path);
			if(iter) {
				// Find type
				Gtk::TreeModel::Row row = *iter;
				const sirEdit::data::Type* type = views->getStaticView().getTypes()[row[typeListModel.data_id]];
				this->currentType = const_cast<Type*>(type);

				// Generate list
				this->fieldListData->clear();
				this->field_lookup.clear();
				{
					unordered_map<const Type*, Gtk::TreeModel::Row> tmp_data;
					buildFieldsLayer(*type, tmp_data);
				}
				this->view_right.expand_all();
			}
		}

		void event_type_changed(Glib::ustring i, TYPE_STATE state) {

		}

		void event_field_changed(Glib::ustring id, FIELD_STATE state) {
			// Find row
			auto row = this->fieldListData->get_iter(id);
			if(!row)
				throw; // This should NEVER happen!

			// Find field
			auto field = this->field_lookup.find(static_cast<string>(row->get_value(fieldListModel.data_name)));
			if(field == this->field_lookup.end())
				throw; // That should NEVER happen!

			// Set field state
			views->setFieldStatus(this->tool, *(this->currentType), *(field->second), state, [this, &id](const Type& type, const Field& field, FIELD_STATE tool_state, FIELD_STATE type_state) -> void {
				this->fieldUpdate(*(this->fieldListData->get_iter(id)), tool_state, type_state);
			}, [](const Type& type, FIELD_STATE tool_state, FIELD_STATE set_state) -> void {

			});
		}

	public:
		Tab(Tool& tool) : tool(tool), HPaned() {
			// General
			const View& view = views->getStaticView();

			// Generate panes
			this->scrollable_left.set_size_request(300, 200);
			this->scrollable_left.add(this->view_left);
			this->pack1(scrollable_left, false, false);
			this->scrollable_right.add(this->view_right);
			this->pack2(scrollable_right, true, false);

			// Types list
			{
				// Generate tree storage
				this->typeListData = Gtk::TreeStore::create(typeListModel);
				this->typeListData->set_sort_column(typeListModel.data_name, Gtk::SortType::SORT_ASCENDING);
				if(!this->typeListData)
					throw; // That should NEVER happen!

				// Generate type tree
				{
					for(auto& i : view.getBaseTypes()) {
						Gtk::TreeStore::Row tmp = *(typeListData->append());
						tmp[typeListModel.data_id] = i->getID();
						tmp[typeListModel.data_name] = i->getName();
						this->buildTreeSubRows(tmp, *i);
					}
				}

				// Set events
				this->view_left.signal_row_activated().connect([this](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) -> void {
					this->event_type_clicked(path);
				});

				// Show model
				this->view_left.set_model(typeListData);
				this->view_left.set_search_column(typeListModel.data_name);
				{
					auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
					tmp->signal_toggled().connect([this](Glib::ustring test) -> void {
						this->event_type_changed(test, TYPE_STATE::NO);
					});
					tmp->set_property("radio", true);
					this->view_left.append_column("-", *tmp);
					this->view_left.get_column(0)->add_attribute(tmp->property_active(), typeListModel.data_status_no);
				}
				{
					auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
					tmp->signal_toggled().connect([this](Glib::ustring test) -> void {
						this->event_type_changed(test, TYPE_STATE::UNUSED);
					});
					tmp->set_property("radio", true);
					this->view_left.append_column("u", *tmp);
					this->view_left.get_column(1)->add_attribute(tmp->property_active(), typeListModel.data_status_u);
					this->view_left.get_column(1)->add_attribute(tmp->property_inconsistent(), typeListModel.data_status_u_transitive);
				}
				{
					auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
					tmp->signal_toggled().connect([this](Glib::ustring test) -> void {
						this->event_type_changed(test, TYPE_STATE::READ);
					});
					tmp->set_property("radio", true);
					this->view_left.append_column("r", *tmp);
					this->view_left.get_column(2)->add_attribute(tmp->property_active(), typeListModel.data_status_r);
					this->view_left.get_column(2)->add_attribute(tmp->property_inconsistent(), typeListModel.data_status_r_transitive);
				}
				{
					auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
					tmp->signal_toggled().connect([this](Glib::ustring test) -> void {
						this->event_type_changed(test, TYPE_STATE::UNUSED);
					});
					tmp->set_property("radio", true);
					this->view_left.append_column("w", *tmp);
					this->view_left.get_column(3)->add_attribute(tmp->property_active(), typeListModel.data_status_w);
					this->view_left.get_column(3)->add_attribute(tmp->property_inconsistent(), typeListModel.data_status_w_transitive);
				}
				{
					auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
					tmp->signal_toggled().connect([this](Glib::ustring test) -> void {
						this->event_type_changed(test, TYPE_STATE::UNUSED);
					});
					tmp->set_property("radio", true);
					this->view_left.append_column("d", *tmp);
					this->view_left.get_column(4)->add_attribute(tmp->property_active(), typeListModel.data_status_d);
					this->view_left.get_column(4)->add_attribute(tmp->property_inconsistent(), typeListModel.data_status_d_transitive);
				}
				this->view_left.append_column("Types", typeListModel.data_name);

				// Display properties
				auto nameColumn = this->view_left.get_column(5);
				this->view_left.set_activate_on_single_click(true);
				this->view_left.expand_all();
				this->view_left.set_expander_column(*nameColumn);
			}

			// Field list
			{
				this->fieldListData = Gtk::TreeStore::create(fieldListModel);
				this->fieldListData->set_sort_column(fieldListModel.data_sort_name, Gtk::SortType::SORT_ASCENDING);
				this->view_right.set_model(fieldListData);
				this->view_right.set_search_column(fieldListModel.data_sort_name);
				{
					auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
					tmp->signal_toggled().connect([this](Glib::ustring id) -> void {
						this->event_field_changed(id, FIELD_STATE::NO);
					});
					tmp->set_property("radio", true);
					this->view_right.append_column("-", *tmp);
					this->view_right.get_column(0)->add_attribute(tmp->property_active(), fieldListModel.data_status_no);
					this->view_right.get_column(0)->add_attribute(tmp->property_inconsistent(), fieldListModel.data_status_no_transitive);
					this->view_right.get_column(0)->add_attribute(tmp->property_activatable(),fieldListModel.data_status_active);
				}
				{
					auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
					tmp->signal_toggled().connect([this](Glib::ustring id) -> void {
						this->event_field_changed(id, FIELD_STATE::UNUSED);
					});
					tmp->set_property("radio", true);
					this->view_right.append_column("u", *tmp);
					this->view_right.get_column(1)->add_attribute(tmp->property_active(), fieldListModel.data_status_u);
					this->view_right.get_column(1)->add_attribute(tmp->property_inconsistent(), fieldListModel.data_status_u_transitive);
					this->view_right.get_column(1)->add_attribute(tmp->property_activatable(),fieldListModel.data_status_active);
				}
				{
					auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
					tmp->signal_toggled().connect([this](Glib::ustring id) -> void {
						this->event_field_changed(id, FIELD_STATE::READ);
					});
					tmp->set_property("radio", true);
					this->view_right.append_column("r", *tmp);
					this->view_right.get_column(2)->add_attribute(tmp->property_active(), fieldListModel.data_status_r);
					this->view_right.get_column(2)->add_attribute(tmp->property_inconsistent(), fieldListModel.data_status_r_transitive);
					this->view_right.get_column(2)->add_attribute(tmp->property_activatable(),fieldListModel.data_status_active);
				}
				{
					auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
					tmp->signal_toggled().connect([this](Glib::ustring id) -> void {
						this->event_field_changed(id, FIELD_STATE::WRITE);
					});
					tmp->set_property("radio", true);
					this->view_right.append_column("w", *tmp);
					this->view_right.get_column(3)->add_attribute(tmp->property_active(), fieldListModel.data_status_w);
					this->view_right.get_column(3)->add_attribute(tmp->property_inconsistent(), fieldListModel.data_status_w_transitive);
					this->view_right.get_column(3)->add_attribute(tmp->property_activatable(),fieldListModel.data_status_active);
				}
				{
					auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
					tmp->signal_toggled().connect([this](Glib::ustring id) -> void {
						this->event_field_changed(id, FIELD_STATE::CREATE);
					});
					tmp->set_property("radio", true);
					this->view_right.append_column("e", *tmp);
					this->view_right.get_column(4)->add_attribute(tmp->property_active(), fieldListModel.data_status_e);
					this->view_right.get_column(4)->add_attribute(tmp->property_inconsistent(), fieldListModel.data_status_e_transitive);
					this->view_right.get_column(4)->add_attribute(tmp->property_activatable(),fieldListModel.data_status_active);
				}
				this->view_right.append_column("Name", fieldListModel.data_name);
				this->view_right.set_expander_column(*(this->view_right.get_column(5)));
			}
		}
};
//
// Show widget
//
extern Gtk::Widget* sirEdit::gui::createToolEdit(std::string name) {
	// Search tool
	Tool* tool = nullptr;
	for(auto& i : views->getStaticView().getTools())
		if(i.getName() == name)
			tool = const_cast<Tool*>(&i);
	if(tool == nullptr)
		throw; // That should NEVER happen.

	// Create tab
	Tab* tab =  new Tab(*tool);
	tab->show_all();
	return tab;
}
