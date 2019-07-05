#include <gtkmm.h>
#include <sirEdit/main.hpp>
#include <sirEdit/data/skillInclude.hpp>
#include <cstdio>
#include <sstream>
#include <fstream>
#include "startupWindow.hpp"


extern std::string sirEdit_startupWindow_glade;

// INFO: Hack to to protect crashes after main program
static Glib::RefPtr<Gtk::Builder>* _startUpBuild = new Glib::RefPtr<Gtk::Builder>();
static Glib::RefPtr<Gtk::Builder>& startUpBuild = *_startUpBuild;

extern void sirEdit::gui::runStartupWindow() {
	// TODO: Checks if all exists

	// Load file
	if(!startUpBuild)
		startUpBuild = Gtk::Builder::create_from_string(sirEdit_startupWindow_glade);

	// Window
	Gtk::ApplicationWindow* awindow;
	{
		startUpBuild->get_widget("StartupWindow", awindow);
		awindow->show_all();
		sirEdit::mainApplication->add_window(*awindow);
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
		auto tmp_label = new Gtk::Label("skill.sir");
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
	{
		Gtk::Button* button;
		Gtk::Button* doImport;
		Gtk::ApplicationWindow* window;
		Gtk::FileChooserButton* importFile;
		Gtk::FileChooserButton* importFolder;
		startUpBuild->get_widget("ImportSKILL", button);
		startUpBuild->get_widget("doImport", doImport);
		startUpBuild->get_widget("ImportSKilLWindow", window);
		startUpBuild->get_widget("importFile", importFile);
		startUpBuild->get_widget("importFolder", importFolder);

		// Activation button
		button->signal_clicked().connect([window, doImport, importFile, importFolder]() -> void {
			window->show_all();

			// Set properties
			doImport->set_sensitive(false);
			importFile->set_sensitive(false);
		});
		importFolder->signal_file_set().connect([importFile, doImport]()-> void {
			importFile->set_sensitive(true);
			doImport->set_sensitive(false);
		});
		importFile->signal_file_set().connect([doImport]()-> void {
			doImport->set_sensitive(true);
		});
		doImport->signal_clicked().connect([importFolder, importFile, window, awindow
]() -> void {
			// Prepare
			std::string folderName = tmpnam(nullptr);
			data::SKilL_Include inc(folderName);
			mkdir(folderName.c_str(), 0777);

			// Download files
			try {
				inc.parse(importFile->get_filename(), [importFolder](std::string file) -> std::string {
					auto ioRemote = importFolder->get_file()->get_child(file)->read();
					std::stringstream tmp;
					char buffer[256];
					int readed;
					while((readed = ioRemote->read(buffer, 256)) != 0)
						tmp.write(buffer, readed);
					ioRemote->close();
					return tmp.str();
				});
				inc.convert(importFile->get_filename());
			}
			catch(...) {
				Gtk::MessageDialog* md = Gtk::manage(new Gtk::MessageDialog("Import was not possible."));
				md->show_all();
				return;
			}

			// User should save the file
			auto chooser = Gtk::FileChooserNative::create("SIR-Files", *window, Gtk::FILE_CHOOSER_ACTION_SAVE);
			switch(chooser->run()) {
				case Gtk::RESPONSE_ACCEPT:
					// Copy file
					{
						char buffer[256];
						int readed;
						std::ifstream inLocal(folderName + "/spec.sir", std::ios::in | std::ios::binary);
						auto tmp = chooser->get_file()->replace();
						while((readed = inLocal.readsome(buffer, 256)) != 0)
							tmp->write(buffer, readed);
						tmp->close();
					}

					// Open main window
					window->close();
					sirEdit::loadFile(awindow, chooser.get());
					break;
			}
		});
	}

	// Open sir file
	{
		Gtk::Button* button;
		startUpBuild->get_widget("OpenSir", button);
		button->signal_clicked().connect([awindow]() -> void {
			auto chooser = Gtk::FileChooserNative::create("SIR-Files", *awindow, Gtk::FILE_CHOOSER_ACTION_OPEN);
			switch(chooser->run()) {
				case Gtk::RESPONSE_ACCEPT:
					sirEdit::loadFile(awindow, chooser.get());
					break;
			}
		});
	}
}
