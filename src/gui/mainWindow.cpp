#include <gtkmm.h>
#include "mainWindow.hpp"
#include <sirEdit/main.hpp>

#include <list>
#include <unordered_map>

using namespace std;
using namespace sirEdit;
using namespace sirEdit::data;
using namespace sirEdit::gui;


class MainWindow {
	private:
		Glib::RefPtr<Gtk::Builder> __builder;

		shared_ptr<Serializer> __serializer;
		Glib::RefPtr<Gio::File> __file;
		HistoricalView __historicalView;

		unordered_map<string, int> __tabs;
		Gtk::Notebook* __notebook;

		Gtk::ListBox* __toolsList;
		Gtk::Popover* __toolsPopover;

		//
		// Tab management
		//
		void __create_tab(Tool& tool) {
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
			Gtk::Widget* content = createToolEdit(tool.getName(), this->__historicalView);
			auto tmp = this->__notebook->append_page(*content, *labelBox);
			this->__tabs[tool.getName()] = tmp;
			this->__notebook->set_current_page(tmp);
		}

		//
		// Event
		//
		void __event_toolButton_click() {
			// Clear list
			{
				auto tmp = this->__toolsList->get_children();
				for(auto i : tmp) {
					this->__toolsList->remove(*i);
					delete i;
				}
			}

			// Rebuild list
			{
				sirEdit::data::View view = this->__historicalView.getStaticView();
				auto& tools = view.getTools();
				size_t pos = 0;
				for(auto& i : tools) {
					Gtk::Button* name = new Gtk::Button(i.getName());
					name->set_property("relief", Gtk::RELIEF_NONE);
					name->signal_clicked().connect([&i, this]() -> void {
						if(this->__tabs.find(i.getName()) == this->__tabs.end())
							this->__create_tab(const_cast<Tool&>(i));
						else
							this->__notebook->set_current_page(this->__tabs[i.getName()]);
						this->__toolsPopover->hide();
					});
					this->__toolsList->insert(*name, pos);
					pos++;
				}
			}

			// Show
			this->__toolsPopover->show_all();
		}

	public:
		MainWindow(shared_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file) : __historicalView(serializer->getView()) {
			// Builder
			this->__builder = Gtk::Builder::create_from_file("data/gui/mainWindow.glade");

			// Gen historical view
			this->__serializer = serializer;
			this->__file = file;

			// Notebook
			this->__builder->get_widget("Notebook", this->__notebook);

			// Tools pop-up
			{
				Gtk::Button* toolsButton;
				this->__builder->get_widget("ToolsButton", toolsButton);
				this->__builder->get_widget("ToolsPopup", this->__toolsPopover);
				this->__builder->get_widget("ToolsList", this->__toolsList);

				toolsButton->signal_clicked().connect([this]() -> void {
					this->__event_toolButton_click();
				});
			}

			// New tool
			{
				Gtk::Button* newToolButton;
				Gtk::Dialog* newToolDialog;
				Gtk::Entry* toolName;
				Gtk::Button* toolFinish;
				Gtk::Button* toolExit;
				this->__builder->get_widget("ToolAddButton", newToolButton);
				this->__builder->get_widget("NewToolDialog", newToolDialog);
				this->__builder->get_widget("ToolName", toolName);
				this->__builder->get_widget("ToolNewAdd", toolFinish);
				this->__builder->get_widget("ToolNewExit", toolExit);

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
					auto updateCheck = [this, toolName, newToolDialog, toolFinish]() -> void {
						string text = toolName->get_text();
						if(text == "") {
							toolFinish->set_sensitive(false);
							return;
						}

						for(auto i : this->__historicalView.getStaticView().getTools())
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
				toolFinish->signal_clicked().connect([this, newToolDialog, toolName]() -> void {
					newToolDialog->hide();
					this->__historicalView.addTool({toolName->get_text()});
					// TODO: Open new tool view
				});
			}

			// Make window visible
			{
				Gtk::ApplicationWindow* window;
				this->__builder->get_widget("mainWindow", window);
				window->show_all();
				sirEdit::mainApplication->add_window(*window);
			}
		}
};

static shared_ptr<MainWindow> mainWindow;

extern void sirEdit::gui::openMainWindow(shared_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file) {
	mainWindow = move(make_shared<MainWindow>(serializer, file));
}
