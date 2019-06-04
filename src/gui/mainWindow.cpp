#include <gtkmm.h>
#include "mainWindow.hpp"
#include <sirEdit/main.hpp>

extern std::string sirEdit_mainWindow_glade;

#include <list>
#include <unordered_map>

using namespace std;
using namespace sirEdit;
using namespace sirEdit::data;
using namespace sirEdit::gui;

class TmpModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<bool> data_used;
		Gtk::TreeModelColumn<bool> data_active;

		TmpModel() {
			this->add(data_used);
			this->add(data_active);
			this->add(data_name);
		}
};
//static TmpModel tmpModel;

class EditTool : public Gtk::Popover {
	private:
		Gtk::VBox layer;
		Gtk::Entry name;
		Gtk::Entry description;
		Gtk::Entry cmd;
		Gtk::Button okaybutton = move(Gtk::Button(Gtk::StockID("gtk-apply")));

		Tool* tool;
		Transactions& transactions;

	public:
		EditTool(Tool* tool, Transactions& transactions) : tool(tool), transactions(transactions) {
			// Basic layout
			this->add(this->layer);
			this->layer.pack_start(this->name, true, false);
			this->layer.pack_start(this->description, true, false);
			this->layer.pack_start(this->cmd, true, false);
			this->layer.pack_start(this->okaybutton, true, false);

			// Set base config
			this->name.set_placeholder_text("Name");
			this->description.set_placeholder_text("Description");
			this->cmd.set_placeholder_text("Command-Line");

			// Set arguments
			this->name.set_text(tool->getName());
			this->description.set_text(tool->getCommand());
			this->cmd.set_text(tool->getCommand());

			// Signals
			this->name.signal_changed().connect([this]() -> void {
				string name = this->name.get_text();
				if(name.empty()) {
					this->okaybutton.set_sensitive(false);
					return;
				}
				for(auto& i : this->transactions.getData().getTools()) {
					if(i->getName() == name && name != this->tool->getName()) {
						this->okaybutton.set_sensitive(false);
						return;
					}
				}
				this->okaybutton.set_sensitive(true);
				return;
			});
			this->okaybutton.signal_clicked().connect([this]() -> void {
				this->hide();
				this->remove();
				//TODO: Make operation
			});
		}
};

class MainWindow {
	private:
		Glib::RefPtr<Gtk::Builder> __builder;

		unique_ptr<Serializer> __serializer;
		Glib::RefPtr<Gio::File> __file;
		Transactions __transitions;

		unordered_map<string, int> __tabs;
		Gtk::Notebook* __notebook;

		Gtk::ListBox* __toolsList;
		Gtk::Popover* __toolsPopover;

		//
		// Tab management
		//
		void __create_tab(Tool& tool) {
			Gtk::HBox* labelBox = Gtk::manage(new Gtk::HBox());
			Gtk::Label* label = Gtk::manage(new Gtk::Label(tool.getName()));
			Gtk::Image* closeImage = Gtk::manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_BUTTON));
			Gtk::Button* closeButton = Gtk::manage(new Gtk::Button());
			// TODO: Close tab
			closeButton->add(*closeImage);
			closeButton->set_property("relief", Gtk::RELIEF_NONE);
			labelBox->pack_start(*label);
			labelBox->pack_end(*closeButton);
			labelBox->show_all();
			Gtk::Widget* content = createToolEdit(tool.getName(), this->__transitions);
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
				const sirEdit::data::Serializer& view = this->__transitions.getData();
				auto& tools = view.getTools();
				size_t pos = 0;
				for(auto& i : tools) {
					Gtk::VBox* main = Gtk::manage(new Gtk::VBox());
					{
						Gtk::HBox* top = Gtk::manage(new Gtk::HBox());
						{
							Gtk::Label* tmp = Gtk::manage(new Gtk::Label(i->getName()));
							tmp->set_alignment(Gtk::ALIGN_START, Gtk::ALIGN_CENTER);
							Gtk::Button* button = Gtk::manage(new Gtk::Button());
							button->set_image(*tmp);
							tmp->show_all();
							top->pack_start(*(button), true, true);
							button->set_relief(Gtk::RELIEF_NONE);
							button->signal_clicked().connect([&i, this]() -> void {
								if(this->__tabs.find(i->getName()) == this->__tabs.end())
									this->__create_tab(*const_cast<Tool*>(i));
								else
									this->__notebook->set_current_page(this->__tabs[i->getName()]);
								this->__toolsPopover->hide();
							});
						}
						{
							Gtk::Button* button = Gtk::manage(new Gtk::Button());
							button->set_relief(Gtk::RELIEF_NONE);
							button->set_image(*(Gtk::manage(new Gtk::Image(Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_BUTTON))));
							top->pack_start(*button, false, true);
						}
						{
							Gtk::Button* button = Gtk::manage(new Gtk::Button());
							button->signal_clicked().connect([button, i, this]() -> void {
								EditTool* editTool = Gtk::manage(new EditTool(i, this->__transitions));
								editTool->set_relative_to(*button);
								editTool->show_all();
							});
							button->set_relief(Gtk::RELIEF_NONE);
							button->set_image(*(Gtk::manage(new Gtk::Image(Gtk::Stock::EDIT, Gtk::ICON_SIZE_BUTTON))));
							top->pack_start(*button, false, true);
						}
						{
							Gtk::Button* button = Gtk::manage(new Gtk::Button());
							button->set_relief(Gtk::RELIEF_NONE);
							button->set_image(*(Gtk::manage(new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON))));
							top->pack_start(*button, false, true);
						}
						main->pack_start(*top, true, true);
					}
					{
						Gtk::Label* description = Gtk::manage(new Gtk::Label(i->getDescription()));
						description->set_line_wrap_mode(Pango::WrapMode::WRAP_CHAR);
						description->set_alignment(Gtk::ALIGN_START, Gtk::ALIGN_CENTER);
						description->set_lines(1);
						main->pack_end(*description, true, true);
					}

					this->__toolsList->insert(*main, pos);
					pos++;
				}
			}

			// Show
			this->__toolsPopover->show_all();
		}

	public:
		MainWindow(unique_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file) : __transitions(*serializer) {
			// Builder
			this->__builder = Gtk::Builder::create();
			Gtk::Popover tmp;
			this->__builder->expose_widget("EditTool", tmp);
			if(!this->__builder->add_from_string(sirEdit_mainWindow_glade))
				throw; // That should never happen

			// Gen historical view
			this->__serializer = std::move(serializer);
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
				Gtk::Popover* newToolDialog;
				Gtk::Entry* toolName;
				Gtk::Entry* toolDescription;
				Gtk::Entry* toolCommand;
				Gtk::Button* toolFinish;
				this->__builder->get_widget("ToolAddButton", newToolButton);
				this->__builder->get_widget("AddToolDialog", newToolDialog);
				this->__builder->get_widget("AddToolName", toolName);
				this->__builder->get_widget("AddToolDescription", toolDescription);
				this->__builder->get_widget("AddToolCMD", toolCommand);
				this->__builder->get_widget("AddToolDo", toolFinish);

				// New dialog
				{
					newToolButton->signal_clicked().connect([newToolDialog, toolName, toolFinish, toolDescription, toolCommand]() -> void {
						// Reset
						toolName->set_text("");
						toolDescription->set_text("");
						toolCommand->set_text("");
						toolFinish->set_sensitive(false);

						// Show
						newToolDialog->show_all();
					});
				}

				// Dialog update checkers
				{
					auto updateCheck = [this, toolName, toolFinish]() -> void {
						string text = toolName->get_text();
						if(text == "") {
							toolFinish->set_sensitive(false);
							return;
						}

						for(auto i : this->__transitions.getData().getTools())
							if(i->getName() == text) {
								toolFinish->set_sensitive(false);
								return;
							}

						toolFinish->set_sensitive(true);
						return;
					};
					toolName->signal_changed().connect(updateCheck);
				}

				// Dialog finished
				toolFinish->signal_clicked().connect([this, newToolDialog, toolName, toolDescription, toolCommand]() -> void {
					newToolDialog->hide();
					const Tool* tool = this->__transitions.addTool({toolName->get_text(), toolDescription->get_buffer()->get_text(), toolCommand->get_buffer()->get_text()});
					this->__create_tab(*const_cast<Tool*>(tool));
				});
			}

			{
				this->__notebook->append_page(*(Gtk::manage(createOverview(this->__transitions))), "Overview");
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

extern void sirEdit::gui::openMainWindow(std::unique_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file) {
	mainWindow = move(make_shared<MainWindow>(std::move(serializer), file));
}
