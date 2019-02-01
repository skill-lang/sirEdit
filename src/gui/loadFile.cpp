#include <cstdio>
#include <fstream>
#include <future>
#include <iostream>
#include <sirEdit/main.hpp>
#include <sirEdit/data/serialize.hpp>

#include "mainWindow.hpp"

using namespace std;

static thread* _loader_thread = new thread();
static thread& loader_thread = *_loader_thread;

inline void loadFileThread(Gtk::Window* mainWindow, Gtk::Window* popup, Glib::RefPtr<Gio::File> file) {
	// TODO: Checks and crashes

	// Create tmpfile
	string fileName = tmpnam(nullptr); // TODO: replace tmpnam
	FILE* output = fopen(fileName.c_str(), "w");

	// Read data
	{
		char* buffer = new char[1024];
		auto io = file->read();
		gsize readed;
		while((readed = io->read(buffer, 1024)) != 0) {
			fwrite(buffer, 1, readed, output);
		}
		io->close();
		fclose(output);
		delete[] buffer;
	}

	// Open file
	auto serializer = sirEdit::data::Serializer::openFile(fileName);

	// Send update
	sirEdit::runInGui([mainWindow, popup, serializer, file]() -> void {
		// Open windows
		sirEdit::gui::openMainWindow(serializer, file);

		// Close windows
		if(popup)
			popup->close();
		if(mainWindow) {
			sirEdit::mainApplication->remove_window(*mainWindow);
			mainWindow->close();
		}
	});
}

extern void sirEdit::loadFile(Gtk::Window* window, Gtk::FileChooserNative* chooser) {
	// TODO: Popup info

	// Start loader
	auto files = chooser->get_files();
	if(files.size() != 1)
		throw; // TODO: Show error
	auto file = files[0];
	loader_thread = move(thread([window, file]() -> void { loadFileThread(window, nullptr, file); }));
}
