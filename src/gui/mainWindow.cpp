#include <gtkmm.h>
#include "mainWindow.hpp"
#include <sirEdit/main.hpp>

#include <iostream>

using namespace std;


// INFO: Hack to to protect crashes after main program
static Glib::RefPtr<Gtk::Builder>* _mainWindowBuild = new Glib::RefPtr<Gtk::Builder>();
static Glib::RefPtr<Gtk::Builder>& mainWindowBuild = *_mainWindowBuild;

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


extern void sirEdit::gui::openMainWindow(shared_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file) {
	// Load window when required
	if(!mainWindowBuild)
		mainWindowBuild = Gtk::Builder::create_from_file("data/gui/mainWindow.glade");

	// Types and field lists
	{
		Gtk::TreeView* treeView;
		mainWindowBuild->get_widget("ClassList", treeView);
		typeListData = Gtk::TreeStore::create(typeListModel);
		typeListData->set_sort_column(typeListModel.data_name, Gtk::SortType::SORT_ASCENDING);
		if(!typeListData)
			throw;

		size_t counter = 0;
		for(auto& i : serializer->getView().getTypes()) {
			Gtk::TreeStore::Row tmp = *(typeListData->append());
			tmp[typeListModel.data_id] = counter;
			tmp[typeListModel.data_name] = i->getName();
			counter++;
		}

		treeView->set_model(typeListData);
		treeView->set_search_column(typeListModel.data_name);
		treeView->append_column("Types", typeListModel.data_name);
		treeView->set_activate_on_single_click(true);

		Gtk::TreeView* fields;
		mainWindowBuild->get_widget("ItemList", fields);
		fieldListData = Gtk::TreeStore::create(fieldListModel);
		fieldListData->set_sort_column(fieldListModel.data_name, Gtk::SortType::SORT_ASCENDING);
		fields->set_model(fieldListData);
		fields->set_search_column(fieldListModel.data_name);
		fields->append_column("Name", fieldListModel.data_name);

		treeView->signal_row_activated().connect([serializer, fields](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) -> void {
			Gtk::TreeModel::iterator iter = typeListData->get_iter(path);
			if(iter) {
				Gtk::TreeModel::Row row = *iter;
				fieldListData->clear();
				sirEdit::data::TypeWithFields* type = static_cast<sirEdit::data::TypeWithFields*>(serializer->getView().getTypes()[row[typeListModel.data_id]].get());
				for(auto& i: type->getFields()) {
					Gtk::TreeStore::Row tmp = *(fieldListData->append());
					tmp[fieldListModel.data_name] = i.getName();
				}
			}
		});
	}

	// Window
	Gtk::ApplicationWindow* window;
	{
		mainWindowBuild->get_widget("mainWindow", window);
		window->show_all();
		sirEdit::mainApplication->add_window(*window);
	}
}
