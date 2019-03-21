#include "mainWindow.hpp"
#include <gtkmm.h>
#include <unordered_map>

#include <iostream>
#include <sirEdit/main.hpp>

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
			this->add(data_status_r);
			this->add(data_status_w);
			this->add(data_status_d);
			this->add(data_name);
		}
};
static TypeListModel typeListModel;
static Glib::RefPtr<Gtk::TreeStore> typeListData;

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
static Glib::RefPtr<Gtk::TreeStore> fieldListData;

inline void buildTreeSubRows(Gtk::TreeStore::Row& row, const sirEdit::data::Type* super) {
	for(auto& i : super->getSubTypes()) {
		Gtk::TreeStore::Row tmp = *(typeListData->append(row.children()));
		tmp[typeListModel.data_id] = i->getID();
		tmp[typeListModel.data_name] = i->getName();
		buildTreeSubRows(tmp, i);
	}
}

inline void fieldUpdate(Gtk::TreeStore::Row row, FIELD_STATE all, FIELD_STATE set) {
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

inline FIELD_STATE getFieldState(const Field& field, const Tool& tool, const Type& type) {
	if(field.getStates().find(const_cast<Tool*>(&tool)) == field.getStates().end())
		return FIELD_STATE::NO;
	auto& tmp = field.getStates().find(const_cast<Tool*>(&tool))->second;
	if(tmp.find(const_cast<Type*>(&type)) == tmp.end())
		return FIELD_STATE::NO;
	return tmp.find(const_cast<Type*>(&type))->second;
}

inline void buildFieldsLayer(const Type* master, const sirEdit::data::Type* type, Tool* tool, std::unordered_map<const sirEdit::data::Type*, Gtk::TreeStore::Row>& types, std::unordered_map<string, sirEdit::data::Field*>& reverse, std::unordered_map<Field*, Gtk::TreeStore::Row>& fieldLookup) {
	// Check if exists double
	if(types.find(type) != types.end())
		return;

	// Call super
	auto super = sirEdit::data::getSuper(*type);
	std::cout << "Super of " << type->getName() << " " << type << ": " << super<< std::endl;
	if(super != nullptr)
		buildFieldsLayer(master, super, tool, types, reverse, fieldLookup);

	// Create type row
	Gtk::TreeStore::Row typeRow;
	if(super != nullptr)
		typeRow = *(fieldListData->append(types[super]->children()));
	else
		typeRow = *(fieldListData->append());
	types[type] = typeRow;
	typeRow[fieldListModel.data_name] = type->getName();
	typeRow[fieldListModel.data_sort_name] = std::string("a_") + type->getName();
	typeRow[fieldListModel.data_status_active] = false;

	// Insert data
	{
		const sirEdit::data::TypeWithFields* fields = *type;
		if(fields != nullptr)
			for(auto& i : fields->getFields()) {
				Gtk::TreeStore::Row tmp = *(fieldListData->append(typeRow.children()));
				tmp[fieldListModel.data_name] = i.getName();
				tmp[fieldListModel.data_sort_name] = std::string("b_") + i.getName();
				tmp[fieldListModel.data_status_active] = true;
				// TODO: set correct state
				reverse[Gtk::TreePath(tmp).to_string()] = const_cast<Field*>(&i);
				fieldLookup[const_cast<Field*>(&i)] = tmp;
				fieldUpdate(tmp, i.getToolType(*tool), getFieldState(i, *tool, *master));
			}
	}
}

//
// Show widget
//
extern Gtk::Widget* sirEdit::gui::createToolEdit(std::string name, const sirEdit::data::View view) {
	// Generate panes
	Gtk::Paned* paned = new Gtk::HPaned();
	Gtk::ScrolledWindow* scrollable_left = new Gtk::ScrolledWindow();
	scrollable_left->set_size_request(300, 200);
	paned->pack1(*scrollable_left, false, false);
	Gtk::ScrolledWindow* scrollable_right = new Gtk::ScrolledWindow();
	paned->pack2(*scrollable_right, true, false);

	// Add tree views
	Gtk::TreeView* view_left = new Gtk::TreeView();
	scrollable_left->add(*view_left);
	Gtk::TreeView* view_right  = new Gtk::TreeView();
	scrollable_right->add(*view_right);

	// Types and field lists
	shared_ptr<std::unordered_map<const sirEdit::data::Type*, Gtk::TreeStore::Row>> field_data = make_shared<std::unordered_map<const sirEdit::data::Type*, Gtk::TreeStore::Row>>();
	shared_ptr<std::unordered_map<Field*, Gtk::TreeStore::Row>> field_row_lookup = make_shared<std::unordered_map<Field*, Gtk::TreeStore::Row>>();
	shared_ptr<std::unordered_map<string, sirEdit::data::Field*>> field_lookup = make_shared<std::unordered_map<string, sirEdit::data::Field*>>();
	shared_ptr<sirEdit::data::Type*> current_type = make_shared<sirEdit::data::Type*>();
	{
		typeListData = Gtk::TreeStore::create(typeListModel);
		typeListData->set_sort_column(typeListModel.data_name, Gtk::SortType::SORT_ASCENDING);
		if(!typeListData)
			throw;

		{
			for(auto& i : view.getBaseTypes()) {
				Gtk::TreeStore::Row tmp = *(typeListData->append());
				tmp[typeListModel.data_id] = i->getID();
				tmp[typeListModel.data_name] = i->getName();
				buildTreeSubRows(tmp, i);
			}
		}

		view_left->set_model(typeListData);
		view_left->set_search_column(typeListModel.data_name);
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_left->append_column("-", *tmp);
			//view_left->get_column(0)->add_attribute(tmp->property_active(), typeListModel.data_status_no);
			tmp->set_property("active", 1);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_left->append_column("u", *tmp);
			//view_left->get_column(1)->add_attribute(tmp->property_active(), typeListModel.data_status_u);
			tmp->set_property("inconsistent", 1);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_left->append_column("r", *tmp);
			view_left->get_column(2)->add_attribute(tmp->property_active(), typeListModel.data_status_r);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_left->append_column("w", *tmp);
			view_left->get_column(3)->add_attribute(tmp->property_active(), typeListModel.data_status_w);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_left->append_column("d", *tmp);
			view_left->get_column(4)->add_attribute(tmp->property_active(), typeListModel.data_status_d);
		}
		view_left->append_column("Types", typeListModel.data_name);
		auto nameColumn = view_left->get_column(5);
		view_left->set_activate_on_single_click(true);
		view_left->expand_all();
		view_left->set_expander_column(*nameColumn);

		fieldListData = Gtk::TreeStore::create(fieldListModel);
		fieldListData->set_sort_column(fieldListModel.data_sort_name, Gtk::SortType::SORT_ASCENDING);
		view_right->set_model(fieldListData);
		view_right->set_search_column(fieldListModel.data_name);
		//Chooser* cl = new Chooser();
		//view_right->append_column("Status", *cl);
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([field_data, field_lookup, field_row_lookup, current_type](Glib::ustring id) -> void {
				// TODO: Correct tool
				auto tmp = field_lookup->find(static_cast<string>(id));
				if(tmp == field_lookup->end())
					throw;
				views->setFieldStatus(*static_cast<Tool*>(nullptr), *(*(current_type.get())), *(tmp->second), sirEdit::data::FIELD_STATE::NO, [&](const Type& type, const Field& field, FIELD_STATE tool_state, FIELD_STATE type_state) -> void {
					fieldUpdate(*fieldListData->get_iter(id), tool_state, type_state);
				}, [](const Type& type, FIELD_STATE tool_state, FIELD_STATE set_state) -> void {

				});
			});
			tmp->set_property("radio", true);
			view_right->append_column("-", *tmp);
			view_right->get_column(0)->add_attribute(tmp->property_active(), fieldListModel.data_status_no);
			view_right->get_column(0)->add_attribute(tmp->property_inconsistent(), fieldListModel.data_status_no_transitive);
			view_right->get_column(0)->add_attribute(tmp->property_activatable(),fieldListModel.data_status_active);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([field_data, field_lookup, field_row_lookup, current_type](Glib::ustring id) -> void {
				// TODO: Correct tool
				auto tmp = field_lookup->find(static_cast<string>(id));
				if(tmp == field_lookup->end())
					throw;
				views->setFieldStatus(*static_cast<Tool*>(nullptr), *(*(current_type.get())), *(tmp->second), sirEdit::data::FIELD_STATE::UNUSED, [&](const Type& type, const Field& field, FIELD_STATE tool_state, FIELD_STATE type_state) -> void {
					fieldUpdate(*fieldListData->get_iter(id), tool_state, type_state);
				}, [](const Type& type, FIELD_STATE tool_state, FIELD_STATE set_state) -> void {

				});
			});
			tmp->set_property("radio", true);
			view_right->append_column("u", *tmp);
			view_right->get_column(1)->add_attribute(tmp->property_active(), fieldListModel.data_status_u);
			view_right->get_column(1)->add_attribute(tmp->property_inconsistent(), fieldListModel.data_status_u_transitive);
			view_right->get_column(1)->add_attribute(tmp->property_activatable(),fieldListModel.data_status_active);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([field_data, field_lookup, field_row_lookup, current_type](Glib::ustring id) -> void {
				// TODO: Correct tool
				auto tmp = field_lookup->find(static_cast<string>(id));
				if(tmp == field_lookup->end())
					throw;
				views->setFieldStatus(*static_cast<Tool*>(nullptr), *(*(current_type.get())), *(tmp->second), sirEdit::data::FIELD_STATE::READ, [&](const Type& type, const Field& field, FIELD_STATE tool_state, FIELD_STATE type_state) -> void {
					fieldUpdate(*fieldListData->get_iter(id), tool_state, type_state);
				}, [](const Type& type, FIELD_STATE tool_state, FIELD_STATE set_state) -> void {

				});
			});
			tmp->set_property("radio", true);
			view_right->append_column("r", *tmp);
			view_right->get_column(2)->add_attribute(tmp->property_active(), fieldListModel.data_status_r);
			view_right->get_column(2)->add_attribute(tmp->property_inconsistent(), fieldListModel.data_status_r_transitive);
			view_right->get_column(2)->add_attribute(tmp->property_activatable(),fieldListModel.data_status_active);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([field_data, field_lookup, field_row_lookup, current_type](Glib::ustring id) -> void {
				// TODO: Correct tool
				auto tmp = field_lookup->find(static_cast<string>(id));
				if(tmp == field_lookup->end())
					throw;
				views->setFieldStatus(*static_cast<Tool*>(nullptr), *(*(current_type.get())), *(tmp->second), sirEdit::data::FIELD_STATE::WRITE, [&](const Type& type, const Field& field, FIELD_STATE tool_state, FIELD_STATE type_state) -> void {
					fieldUpdate(*fieldListData->get_iter(id), tool_state, type_state);
				}, [](const Type& type, FIELD_STATE tool_state, FIELD_STATE set_state) -> void {

				});
			});
			tmp->set_property("radio", true);
			view_right->append_column("w", *tmp);
			view_right->get_column(3)->add_attribute(tmp->property_active(), fieldListModel.data_status_w);
			view_right->get_column(3)->add_attribute(tmp->property_inconsistent(), fieldListModel.data_status_w_transitive);
			view_right->get_column(3)->add_attribute(tmp->property_activatable(),fieldListModel.data_status_active);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([field_data, field_lookup, field_row_lookup, current_type](Glib::ustring id) -> void {
				// TODO: Correct tool
				auto tmp = field_lookup->find(static_cast<string>(id));
				if(tmp == field_lookup->end())
					throw;
				views->setFieldStatus(*static_cast<Tool*>(nullptr), *(*(current_type.get())), *(tmp->second), sirEdit::data::FIELD_STATE::CREATE, [&](const Type& type, const Field& field, FIELD_STATE tool_state, FIELD_STATE type_state) -> void {
					fieldUpdate(*fieldListData->get_iter(id), tool_state, type_state);
				}, [](const Type& type, FIELD_STATE tool_state, FIELD_STATE set_state) -> void {

				});
			});
			tmp->set_property("radio", true);
			view_right->append_column("e", *tmp);
			view_right->get_column(4)->add_attribute(tmp->property_active(), fieldListModel.data_status_e);
			view_right->get_column(4)->add_attribute(tmp->property_inconsistent(), fieldListModel.data_status_e_transitive);
			view_right->get_column(4)->add_attribute(tmp->property_activatable(),fieldListModel.data_status_active);
		}
		view_right->append_column("Name", fieldListModel.data_name);
		view_right->set_expander_column(*(view_right->get_column(5)));

		view_left->signal_row_activated().connect([view, view_right, field_data, field_row_lookup, field_lookup, current_type](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) -> void {
			Gtk::TreeModel::iterator iter = typeListData->get_iter(path);
			if(iter) {
				Gtk::TreeModel::Row row = *iter;
				fieldListData->clear();
				const sirEdit::data::Type* type = view.getTypes()[row[typeListModel.data_id]];
				*(current_type.get()) = const_cast<Type*>(type);
				field_data->clear();
				buildFieldsLayer(type, type, nullptr, *(field_data.get()), *(field_lookup.get()), *(field_row_lookup.get()));
				view_right->expand_all();
			}
		});
	}

	// Return paned
	paned->show_all();
	return paned;
}
