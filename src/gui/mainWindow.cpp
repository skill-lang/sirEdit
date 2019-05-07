#include <gtkmm.h>
#include "mainWindow.hpp"
#include <sirEdit/main.hpp>

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
					Gtk::VBox* main = Gtk::manage(new Gtk::VBox());
					{
						Gtk::HBox* top = Gtk::manage(new Gtk::HBox());
						{
							Gtk::Label* tmp = Gtk::manage(new Gtk::Label(i.getName()));
							tmp->set_alignment(Gtk::ALIGN_START, Gtk::ALIGN_CENTER);
							Gtk::Button* button = Gtk::manage(new Gtk::Button());
							button->set_image(*tmp);
							tmp->show_all();
							top->pack_start(*(button), true, true);
							button->set_relief(Gtk::RELIEF_NONE);
							button->signal_clicked().connect([&i, this]() -> void {
								if(this->__tabs.find(i.getName()) == this->__tabs.end())
									this->__create_tab(const_cast<Tool&>(i));
								else
									this->__notebook->set_current_page(this->__tabs[i.getName()]);
								this->__toolsPopover->hide();
							});
						}
						{
							Gtk::Button* button = Gtk::manage(new Gtk::Button());
							button->set_relief(Gtk::RELIEF_NONE);
							button->set_image(*(Gtk::manage(new Gtk::Image(Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_BUTTON))));
							top->pack_end(*button, false, true);
						}
						{
							Gtk::Button* button = Gtk::manage(new Gtk::Button());
							button->set_relief(Gtk::RELIEF_NONE);
							button->set_image(*(Gtk::manage(new Gtk::Image(Gtk::Stock::PROPERTIES, Gtk::ICON_SIZE_BUTTON))));
							top->pack_end(*button, false, true);
						}
						{
							Gtk::Button* button = Gtk::manage(new Gtk::Button());
							button->set_relief(Gtk::RELIEF_NONE);
							button->set_image(*(Gtk::manage(new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON))));
							top->pack_end(*button, false, true);
						}
						main->pack_start(*top, true, true);
					}
					{
						Gtk::Label* description = Gtk::manage(new Gtk::Label(i.getDescription()));
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
				Gtk::TextView* toolDescription;
				Gtk::TextView* toolCommand;
				this->__builder->get_widget("ToolAddButton", newToolButton);
				this->__builder->get_widget("NewToolDialog", newToolDialog);
				this->__builder->get_widget("ToolName", toolName);
				this->__builder->get_widget("ToolNewAdd", toolFinish);
				this->__builder->get_widget("ToolNewExit", toolExit);
				this->__builder->get_widget("ToolDescription", toolDescription);
				this->__builder->get_widget("ToolCommand", toolCommand);

				// New dialog / close
				{
					newToolButton->signal_clicked().connect([newToolDialog, toolName, toolFinish, toolDescription, toolCommand]() -> void {
						toolName->set_text("");
						toolDescription->set_buffer(Gtk::TextBuffer::create());
						toolCommand->set_buffer(Gtk::TextBuffer::create());
						toolFinish->set_sensitive(false);
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
				toolFinish->signal_clicked().connect([this, newToolDialog, toolName, toolDescription, toolCommand]() -> void {
					newToolDialog->hide();
					this->__historicalView.addTool({toolName->get_text(), toolDescription->get_buffer()->get_text(), toolCommand->get_buffer()->get_text()});
					// TODO: Open new tool view
				});
			}

			{
				this->__notebook->append_page(*(Gtk::manage(createOverview(this->__historicalView))), "Overview");
			}

			// TODO: Remove prototype
			if(false)
			{
				{
					Gtk::TreeView* tools;
					TmpModel* tmpModel = new TmpModel;
					this->__builder->get_widget("OverviewTools", tools);
					{
						auto tmp = Gtk::TreeStore::create(*tmpModel);
						auto tmp2 = *tmp->append();
						tmp2[tmpModel->data_name] = "ToolA";
						tmp2[tmpModel->data_used] = true;
						tmp2[tmpModel->data_active] = true;
						auto tmp3 = *tmp->append();
						tmp3[tmpModel->data_name] = "ToolB";
						tmp3[tmpModel->data_used] = true;
						tmp3[tmpModel->data_active] = false;
						auto tmp4 = *tmp->append();
						tmp4[tmpModel->data_name] = "ToolC";
						tmp4[tmpModel->data_used] = false;
						tmp4[tmpModel->data_active] = false;
						tools->set_model(tmp);
					}
					{
						auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
						tools->append_column("Active", *tmp);
						tools->get_column(0)->add_attribute(tmp->property_active(), tmpModel->data_active);
					}
					{
						auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
						tmp->set_activatable(false);
						tools->append_column("Used", *tmp);
						tools->get_column(1)->add_attribute(tmp->property_active(), tmpModel->data_used);
					}
					{
						tools->append_column("Tool", tmpModel->data_name);
					}
				}
				{
					Gtk::TreeView* tools;
					TmpModel* tmpModel = new TmpModel;
					this->__builder->get_widget("OverviewTypes", tools);
					{
						auto tmp = Gtk::TreeStore::create(*tmpModel);
						auto tmp2 = *tmp->append();
						tmp2[tmpModel->data_name] = "TypeExample";
						tmp2[tmpModel->data_used] = true;
						tmp2[tmpModel->data_active] = true;
						tools->set_model(tmp);
					}
					{
						auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
						tools->append_column("Active", *tmp);
						tools->get_column(0)->add_attribute(tmp->property_active(), tmpModel->data_active);
					}
					{
						auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
						tmp->set_activatable(false);
						tools->append_column("Used", *tmp);
						tools->get_column(1)->add_attribute(tmp->property_active(), tmpModel->data_used);
					}
					{
						tools->append_column("Type", tmpModel->data_name);
					}
				}
				{
					Gtk::TreeView* tools;
					TmpModel* tmpModel = new TmpModel;
					this->__builder->get_widget("OverviewFields", tools);
					{
						auto tmp = Gtk::TreeStore::create(*tmpModel);
						auto tmp2 = *tmp->append();
						tmp2[tmpModel->data_name] = "exampleField";
						tmp2[tmpModel->data_used] = true;
						tmp2[tmpModel->data_active] = true;
						tools->set_model(tmp);
					}
					{
						auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
						tools->append_column("Active", *tmp);
						tools->get_column(0)->add_attribute(tmp->property_active(), tmpModel->data_active);
					}
					{
						auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
						tmp->set_activatable(false);
						tools->append_column("Used", *tmp);
						tools->get_column(1)->add_attribute(tmp->property_active(), tmpModel->data_used);
					}
					{
						tools->append_column("Field", tmpModel->data_name);
					}
				}
				{
					Gtk::Button* button;
					Gtk::Popover* over;
					this->__builder->get_widget("ErrorButton", button);
					this->__builder->get_widget("ErrorPopover", over);
					button->signal_clicked().connect([over]() -> void {
						over->show_all();
					});
				}
				{
					Gtk::Button* button;
					Gtk::PopoverMenu* over;
					this->__builder->get_widget("MenuButton", button);
					this->__builder->get_widget("Menu", over);
					button->signal_clicked().connect([over]() -> void {
						over->show_all();
					});
				}
				{
					Gtk::TreeView* tools;
					TmpModel* tmpModel = new TmpModel;
					this->__builder->get_widget("ExportTools", tools);
					{
						auto tmp = Gtk::TreeStore::create(*tmpModel);
						auto tmp2 = *tmp->append();
						tmp2[tmpModel->data_name] = "ToolA";
						tmp2[tmpModel->data_used] = true;
						tmp2[tmpModel->data_active] = true;
						auto tmp3 = *tmp->append();
						tmp3[tmpModel->data_name] = "ToolB";
						tmp3[tmpModel->data_used] = true;
						tmp3[tmpModel->data_active] = true;
						auto tmp4 = *tmp->append();
						tmp4[tmpModel->data_name] = "ToolC";
						tmp4[tmpModel->data_used] = false;
						tmp4[tmpModel->data_active] = true;
						tools->set_model(tmp);
					}
					{
						auto tmp = Gtk::manage(new Gtk::CellRendererToggle());
						tools->append_column("Export", *tmp);
						tools->get_column(0)->add_attribute(tmp->property_active(), tmpModel->data_active);
					}
					{
						tools->append_column("Tool", tmpModel->data_name);
					}
				}
				{
					Gtk::Dialog* dialog;
					Gtk::Button* button;
					this->__builder->get_widget("ExportButton", button);
					this->__builder->get_widget("Export", dialog);
					button->signal_clicked().connect([dialog]()-> void{
						dialog->show_all();
					});
				}
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
