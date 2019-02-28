#include <gtkmm.h>
#include "mainWindow.hpp"
#include <sirEdit/main.hpp>

#include <list>

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

static list<sirEdit::data::View> views;

inline void buildTreeSubRows(Gtk::TreeStore::Row& row, const sirEdit::data::Type* super) {
	for(auto& i : super->getSubTypes()) {
		Gtk::TreeStore::Row tmp = *(typeListData->append(row.children()));
		tmp[typeListModel.data_id] = i->getID();
		tmp[typeListModel.data_name] = i->getName();
		buildTreeSubRows(tmp, i);
	}
}

extern void sirEdit::gui::openMainWindow(shared_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file) {
	// Load window when required
	if(!mainWindowBuild)
		mainWindowBuild = Gtk::Builder::create_from_file("data/gui/mainWindow.glade");

	// First view
	views.push_back(move(serializer->getView()));

	// Types and field lists
	{
		Gtk::TreeView* treeView;
		mainWindowBuild->get_widget("ClassList", treeView);
		typeListData = Gtk::TreeStore::create(typeListModel);
		typeListData->set_sort_column(typeListModel.data_name, Gtk::SortType::SORT_ASCENDING);
		if(!typeListData)
			throw;

		{
			for(auto& i : views.begin()->getBaseTypes()) {
				Gtk::TreeStore::Row tmp = *(typeListData->append());
				tmp[typeListModel.data_id] = i->getID();
				tmp[typeListModel.data_name] = i->getName();
				buildTreeSubRows(tmp, i);
			}
		}

		treeView->set_model(typeListData);
		treeView->set_search_column(typeListModel.data_name);
		treeView->append_column("Types", typeListModel.data_name);
		treeView->set_activate_on_single_click(true);
		treeView->expand_all();
		//treeView->set_expander_column(typeListModel.data_name);

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

	// Tools pop-up
	{
		Gtk::Button* toolsButton;
		Gtk::Popover* toolsPopover;
		Gtk::ListBox* toolsList;
		mainWindowBuild->get_widget("ToolsButton", toolsButton);
		mainWindowBuild->get_widget("ToolsPopup", toolsPopover);
		mainWindowBuild->get_widget("ToolsList", toolsList);

		toolsButton->signal_clicked().connect([toolsList, toolsPopover, serializer]() -> void {
			// Clear list
			{
				auto tmp = toolsList->get_children();
				for(auto i : tmp) {
					toolsList->remove(*i);
					delete i;
				}
			}

			// Rebuild list
			{
				auto& view = (--views.end())->getTools();
				size_t pos = 0;
				for(auto& i : view) {
					Gtk::Label* name = new Gtk::Label(i.getName());
					toolsList->insert(*name, pos);
					pos++;
				}
			}

			// Show
			toolsPopover->show_all();
		});
	}

	// New tool
	{
		Gtk::Button* newToolButton;
		Gtk::Dialog* newToolDialog;
		Gtk::Entry* toolName;
		Gtk::Button* toolFinish;
		Gtk::Button* toolExit;
		mainWindowBuild->get_widget("ToolAddButton", newToolButton);
		mainWindowBuild->get_widget("NewToolDialog", newToolDialog);
		mainWindowBuild->get_widget("ToolName", toolName);
		mainWindowBuild->get_widget("ToolNewAdd", toolFinish);
		mainWindowBuild->get_widget("ToolNewExit", toolExit);

		// New dialog / close
		{
			newToolButton->signal_clicked().connect([newToolDialog, toolName, toolExit]() -> void {
				toolName->set_text("");
				toolExit->set_sensitive(false);
				newToolDialog->show_all();
			});

			auto closeFunc = [newToolDialog]() -> void {
				newToolDialog->hide();
			};
			toolExit->signal_clicked().connect(closeFunc);
		}

		// Dialog update checkers
		{
			auto updateCheck = [toolName, newToolDialog, toolExit]() -> void {
				if(toolName->get_text() == "") {
					toolExit->set_sensitive(false);
					return;
				}

				// TODO: Check if tool exists

				toolExit->set_sensitive(true);
				return;
			};
			toolName->signal_changed().connect(updateCheck);
		}

		// Dialog finished
		toolFinish->signal_clicked().connect([newToolDialog, toolName]() -> void {
			newToolDialog->hide();
			views.push_back(move((--views.end())->addTool({toolName->get_text()})));
			// TODO: Open new tool view
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
