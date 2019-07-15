#include <gtkmm.h>
#include "mainWindow.hpp"
#include <list>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <sirEdit/main.hpp>
#include <sirEdit/data/tools.hpp>
#include <sirEdit/data/specUpdater.hpp>
#include <thread>
#include <iostream>

using namespace std;
using namespace sirEdit;
using namespace sirEdit::data;
using namespace sirEdit::gui;


extern string sirEdit_mainWindow_glade;

//
// Export main
//

class ExportModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<Tool*> data_id;
		Gtk::TreeModelColumn<Glib::ustring> data_name;

		ExportModel() {
			this->add(data_id);
			this->add(data_name);
		}
};
static ExportModel exportModel;

inline void exportExample(Gtk::TreeView* tree) {
	auto model = Gtk::TreeStore::create(exportModel);
	{
		auto tmp = *(model->append());
		tmp[exportModel.data_name] = "-- ALL --";
	}

	{
		auto tmp = *(model->append());
		tmp[exportModel.data_name] = "Werkzeug1";
	}
	tree->set_model(model);
	tree->append_column("Tool", exportModel.data_name);
	tree->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_MULTIPLE);
}

//
// Edit tool
//

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
			this->description.set_text(tool->getDescription());
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
				this->transactions.updateTool(*this->tool, this->name.get_text(), this->description.get_text(), this->cmd.get_text());
			});
		}
};

//
// Main Window
//

class MainWindow {
	private:
		Glib::RefPtr<Gtk::Builder> __builder;

		unique_ptr<Serializer> __serializer;
		Glib::RefPtr<Gio::File> __file;
		Transactions __transitions;

		unordered_map<Tool*, int> __tabs;
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
			Gtk::Widget* content = createToolEdit(tool.getName(), this->__transitions);
			closeButton->add(*closeImage);
			closeButton->set_property("relief", Gtk::RELIEF_NONE);
			closeButton->signal_clicked().connect([this, &tool]() -> void {
				// Remove page
				int tmp = this->__tabs[&tool];
				this->__notebook->remove_page(tmp);
				this->__tabs.erase(&tool);

				// Update tabs
				for(auto& i : this->__tabs)
					if(i.second >= tmp)
						i.second--;
			});
			labelBox->pack_start(*label);
			labelBox->pack_end(*closeButton);
			labelBox->show_all();
			auto tmp = this->__notebook->append_page(*content, *labelBox);
			__tabs[&tool] = tmp;
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
						// new tool
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
								if(this->__tabs.find(i) == this->__tabs.end())
									this->__create_tab(*const_cast<Tool*>(i));
								else
									this->__notebook->set_current_page(this->__tabs[i]);
								this->__toolsPopover->hide();
							});
						}

						// run command
						{
							Gtk::Button* button = Gtk::manage(new Gtk::Button());
							button->set_relief(Gtk::RELIEF_NONE);
							button->set_image(*(Gtk::manage(new Gtk::Image(Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_BUTTON))));
							button->signal_clicked().connect([this, i]() -> void {
								new std::thread([this, i]() -> void {
									std::string specFile = getSavePath() + "/spec_" + i->getName() + ".skill";
									{
										std::string spec = getSpec(i);
										std::ofstream specOut(specFile, ios::binary | ios::out);
										specOut << spec;
										specOut.close();
									}
									std::string cmd = i->parseCMD(specFile);
									std::cout << "COMMAND: " << cmd << std::endl;
									{
										std::ofstream codegen(getSavePath() + "/codegen.jar", ios::binary | ios::out);
										codegen << sirEdit_codegen;
										codegen.close();
									}
									runCommand({"xterm", "-e", cmd + "; sleep 10"}, getSavePath());
								});
							});
							top->pack_start(*button, false, true);
						}

						// edit tool
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

						// remove tool
						{
							Gtk::Button* button = Gtk::manage(new Gtk::Button());
							button->set_relief(Gtk::RELIEF_NONE);
							button->set_image(*(Gtk::manage(new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON))));
							button->signal_clicked().connect([i, this]() -> void {
								if(this->__tabs.find(i) != this->__tabs.end())
									this->__notebook->remove_page(this->__tabs[i]);
								this->__transitions.removeTool(*i);
								this->__event_toolButton_click();
							});
							top->pack_start(*button, false, true);
						}
						main->pack_start(*top, true, true);
					}

					// description
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

		//
		// Import
		//
		// User should save the file
		void __importTools() {
			auto chooser = Gtk::FileChooserNative::create("SIR-File", Gtk::FILE_CHOOSER_ACTION_OPEN);
			switch(chooser->run()) {
				case Gtk::RESPONSE_ACCEPT:
					// Copy file
					std::string folderName = tmpnam(nullptr);
					{
						char buffer[256];
						int readed;
						std::ofstream outLocal(folderName, std::ios::out | std::ios::binary);
						auto tmp = chooser->get_file()->read();
						while((readed = tmp->read(buffer, 256)) != 0)
							outLocal.write(buffer, readed);
						tmp->close();
					}

					// Open file
					{
						auto newSpec = getSir(folderName);
						SpecModify own;
						own.types = this->__serializer->getTypes();
						own.updateFields();
						std::vector<Tool*> toolsToAdd;
						std::vector<const Type*> tmp = {newSpec->getTypes().begin(), newSpec->getTypes().end()};
						for(auto& i  : newSpec->getTools())
							toolsToAdd.push_back(new Tool(move(own.parseTool(*i, tmp))));
						this->__transitions.importTools(toolsToAdd);
					}
			}
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

			// Import
			{
				Gtk::Button* button;
				this->__builder->get_widget("ImportButton", button);
				button->signal_clicked().connect([this]() -> void {
					this->__importTools();
				});
			}

			// Export
			{
				Gtk::Button* button;
				Gtk::Dialog* dialog;
				Gtk::TreeView* tree;
				this->__builder->get_widget("ExportButton", button);
				this->__builder->get_widget("Export", dialog);
				this->__builder->get_widget("ExportTools", tree);
				button->signal_clicked().connect([dialog]() -> void {
					// TODO: reset
					dialog->show_all();
				});
				exportExample(tree);
			}

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
				window->signal_hide().connect([]() -> void {
					doSave();
				});
			}
		}
};

static shared_ptr<MainWindow> mainWindow;

extern void sirEdit::gui::openMainWindow(std::unique_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file) {
	mainWindow = move(make_shared<MainWindow>(std::move(serializer), file));
}
