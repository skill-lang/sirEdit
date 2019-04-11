#include <gtkmm.h>
#include <sirEdit/main.hpp>

#include "startupWindow.hpp"

// INFO: Hack to to protect crashes after main program
static Glib::RefPtr<Gtk::Builder>* _startUpBuild = new Glib::RefPtr<Gtk::Builder>();
static Glib::RefPtr<Gtk::Builder>& startUpBuild = *_startUpBuild;

extern void sirEdit::gui::runStartupWindow() {
	// TODO: Checks if all exists

	// Load file
	if(!startUpBuild)
		startUpBuild = Gtk::Builder::create_from_file("data/gui/startupWindow.glade");

	// Window
	Gtk::ApplicationWindow* window;
	{
		startUpBuild->get_widget("StartupWindow", window);
		window->show_all();
		sirEdit::mainApplication->add_window(*window);
	}

	// Settings
	{
		Gtk::Popover* popover;
		Gtk::Button* button;
		startUpBuild->get_widget("SettingMenu", popover);
		startUpBuild->get_widget("Settings", button);
		button->signal_clicked().connect([popover]() -> void {
			popover->show_all();
			popover->popup();
		});
	}

	// TODO: Merge sir files
	{
		Gtk::Button* button;
		Gtk::ApplicationWindow* window;
		Gtk::ListBox* list;
		startUpBuild->get_widget("Multispec", button);
		startUpBuild->get_widget("MultispecWindow", window);
		startUpBuild->get_widget("MultispecList", list);

		auto tmp = new Gtk::HBox();
		auto tmp_label = new Gtk::Label("/home/marko/Projekte/SkillEdit/sirEdit/skill.sir");
		tmp_label->set_alignment(Gtk::Align::ALIGN_START, Gtk::Align::ALIGN_CENTER);
		tmp->pack_start(*(tmp_label), true, true);
		auto tmpButton = new Gtk::Button();
		tmpButton->set_image(*(new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON)));
		tmpButton->set_relief(Gtk::RELIEF_NONE);

		tmp->pack_end(*(tmpButton), false, true);
		list->append(*tmp);

		button->signal_clicked().connect([window]() -> void {
			window->show_all();
		});
	}

	// TODO: Info

	// TODO: Import skill file

	// Open sir file
	{
		Gtk::Button* button;
		startUpBuild->get_widget("OpenSir", button);
		button->signal_clicked().connect([window]() -> void {
			auto chooser = Gtk::FileChooserNative::create("SIR-Files", *window, Gtk::FILE_CHOOSER_ACTION_OPEN);
			switch(chooser->run()) {
				case Gtk::RESPONSE_ACCEPT:
					sirEdit::loadFile(window, chooser.get());
					break;
			}
		});
	}
}
