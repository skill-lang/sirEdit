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
