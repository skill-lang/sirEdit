#include <cstdio>
#include <fstream>
#include <atomic>
#include <future>
#include <iostream>
#include <thread>
#include <sirEdit/main.hpp>
#include <sirEdit/data/serialize.hpp>

#include "mainWindow.hpp"

#include <iostream>

using namespace std;

extern volatile bool ended;

static thread* _loader_thread = new thread();
static thread& loader_thread = *_loader_thread;

static Glib::RefPtr<Gio::File> outputFile;
static std::string filePath;
static sirEdit::data::Serializer* ser;

std::string sirEdit::getSavePath() {
	return outputFile->get_parent()->get_path();
}

void sirEdit::doSave(bool blocking) {
	// Save function
	auto toRunFunc = []() -> void {
		char buffer[256];
		ifstream input(filePath, ios::binary);
		auto output = outputFile->replace();
		int read;
		while((read = input.readsome(buffer, 256)) > 0) {
			output->write(buffer, read);
		}
		output->close();
		input.close();
	};

	// TODO: Asyc
	ser->prepareSave();
	ser->save();
	toRunFunc();
}

inline void loadFileThread(Gtk::Window* mainWindow, Gtk::Window* popup, Glib::RefPtr<Gio::File> file) {
	outputFile = file;

	// Create tmpfile
	string fileName = tmpnam(nullptr); // TODO: replace tmpnam
	filePath = fileName;
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
	std::unique_ptr<sirEdit::data::Serializer> serializer = std::move(sirEdit::data::getSir(fileName));
	ser = serializer.get();
	std::mutex mutex;
	std::condition_variable cv;
	std::unique_lock<std::mutex> lock(mutex);

	// Send update
	sirEdit::runInGui([mainWindow, popup, &serializer, file, &cv, &mutex]() -> void {
		// Open windows
		std::unique_lock<std::mutex> lock(mutex);
		sirEdit::gui::openMainWindow(std::move(static_cast<std::unique_ptr<sirEdit::data::Serializer>&>(serializer)), file);
		cv.notify_one();

		// Close windows
		if(popup)
			popup->close();
		if(mainWindow) {
			sirEdit::mainApplication->remove_window(*mainWindow);
			mainWindow->close();
		}
	});
	cv.wait(lock);
}

extern void sirEdit::loadFile(Gtk::Window* window, Gtk::FileChooserNative* chooser) {
	// Autosave
	new thread([]() -> void {
		if(ended)
			return;
		runInGui([]() -> void {
			doSave(false);
		});
		sleep(30);
	});

	// Start loader
	auto files = chooser->get_files();
	if(files.size() != 1)
		throw; // TODO: Show error
	auto file = files[0];
	loader_thread = move(thread([window, file]() -> void { loadFileThread(window, nullptr, file); }));
}
