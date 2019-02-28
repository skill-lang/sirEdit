#include "mainWindow.hpp"
#include <gtkmm.h>

//
// Model for type  list
//
class TypeListModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<size_t> data_id;
		Gtk::TreeModelColumn<Glib::ustring> data_name;

		TypeListModel() {
			this->add(data_id);
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
		Gtk::TreeModelColumn<Glib::ustring> data_name;

		FieldListModel() {
			this->add(data_name);
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
		view_left->append_column("Types", typeListModel.data_name);
		auto nameColumn = view_left->get_column(0);
		view_left->set_activate_on_single_click(true);
		view_left->expand_all();
		view_left->set_expander_column(*nameColumn);

		fieldListData = Gtk::TreeStore::create(fieldListModel);
		fieldListData->set_sort_column(fieldListModel.data_name, Gtk::SortType::SORT_ASCENDING);
		view_right->set_model(fieldListData);
		view_right->set_search_column(fieldListModel.data_name);
		view_right->append_column("Name", fieldListModel.data_name);

		view_left->signal_row_activated().connect([view](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) -> void {
			Gtk::TreeModel::iterator iter = typeListData->get_iter(path);
			if(iter) {
				Gtk::TreeModel::Row row = *iter;
				fieldListData->clear();
				sirEdit::data::TypeWithFields* type = static_cast<sirEdit::data::TypeWithFields*>(view.getTypes()[row[typeListModel.data_id]].get());
				for(auto& i: type->getFields()) {
					Gtk::TreeStore::Row tmp = *(fieldListData->append());
					tmp[fieldListModel.data_name] = i.getName();
				}
			}
		});
	}

	// Return paned
	paned->show_all();
	return paned;
}
