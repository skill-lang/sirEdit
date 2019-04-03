#include <gtkmm.h>
#include "mainWindow.hpp"
#include <sirEdit/main.hpp>

#include <list>
#include <unordered_map>

using namespace std;
using namespace sirEdit;
using namespace sirEdit::data;
using namespace sirEdit::gui;


// INFO: Hack to to protect crashes after main program
static Glib::RefPtr<Gtk::Builder>* _mainWindowBuild = new Glib::RefPtr<Gtk::Builder>();
static Glib::RefPtr<Gtk::Builder>& mainWindowBuild = *_mainWindowBuild;


static unordered_map<string, int> tabs;

inline void event_create_tab(Tool& tool, Gtk::Notebook& notebook) {
	Gtk::HBox* labelBox = new Gtk::HBox();
	Gtk::Label* label = new Gtk::Label(tool.getName());
	Gtk::Image* closeImage = new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_BUTTON);
	Gtk::Button* closeButon = new Gtk::Button();
	// TODO: Close tab
	closeButon->add(*closeImage);
	closeButon->set_property("relief", Gtk::RELIEF_NONE);
	labelBox->pack_start(*label);
	labelBox->pack_end(*closeButon);
	labelBox->show_all();
	Gtk::Widget* content = createToolEdit(tool.getName());
	auto tmp = notebook.append_page(*content, *labelBox);
	tabs[tool.getName()] = tmp;
	notebook.set_current_page(tmp);
}

extern void sirEdit::gui::openMainWindow(shared_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file) {
	// Load window when required
	if(!mainWindowBuild)
		mainWindowBuild = Gtk::Builder::create_from_file("data/gui/mainWindow.glade");

	// First view
	views = new sirEdit::data::HistoricalView(move(serializer->getView()));

	// Notebook
	Gtk::Notebook* notebook;
	mainWindowBuild->get_widget("Notebook", notebook);

	// Tools pop-up
	{
		Gtk::Button* toolsButton;
		Gtk::Popover* toolsPopover;
		Gtk::ListBox* toolsList;
		mainWindowBuild->get_widget("ToolsButton", toolsButton);
		mainWindowBuild->get_widget("ToolsPopup", toolsPopover);
		mainWindowBuild->get_widget("ToolsList", toolsList);

		toolsButton->signal_clicked().connect([toolsList, toolsPopover, serializer, notebook]() -> void {
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
				sirEdit::data::View view = views->getStaticView();
				auto& tools = view.getTools();
				size_t pos = 0;
				for(auto& i : tools) {
					Gtk::Button* name = new Gtk::Button(i.getName());
					name->set_property("relief", Gtk::RELIEF_NONE);
					name->signal_clicked().connect([&i, notebook, toolsPopover]() -> void {
						if(tabs.find(i.getName()) == tabs.end())
							event_create_tab(const_cast<Tool&>(i), *notebook);
						else
							notebook->set_current_page(tabs[i.getName()]);
						toolsPopover->hide();
					});
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
			auto updateCheck = [toolName, newToolDialog, toolFinish]() -> void {
				string text = toolName->get_text();
				if(text == "") {
					toolFinish->set_sensitive(false);
					return;
				}

				for(auto i : views->getStaticView().getTools())
					if(i.getName() == text) {
						toolFinish->set_sensitive(false);
						return;
					}

				toolFinish->set_sensitive(true);
				return;
			};
			toolName->signal_changed().connect(updateCheck);
		}

		// Dialog finished
		toolFinish->signal_clicked().connect([newToolDialog, toolName]() -> void {
			newToolDialog->hide();
			views->addTool({toolName->get_text()});
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
