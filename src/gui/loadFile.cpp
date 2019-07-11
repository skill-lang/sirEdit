#include <cstdio>
#include <fstream>
#include <atomic>
#include <fstream>
#include <future>
#include <iostream>
#include <thread>
#include <mutex>
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
static mutex saveMutex;

extern std::string sirEdit::getSavePath() {
	return outputFile->get_parent()->get_path();
}
extern std::string sirEdit::getSirPath() {
	return filePath;
}
extern std::string sirEdit::getSpec(const sirEdit::data::Tool* tool) {
	// Global spec?
	bool wasGlobal = false;
	if(tool == nullptr) {
		wasGlobal = true;
		auto* t = new sirEdit::data::Tool("@@ALL@@", "", "");
		tool = t;
		for(auto& i : ser->getTypes())
			for(auto& j : sirEdit::data::getFields(*i))
				t->setFieldState(*i, j, sirEdit::data::FIELD_STATE::READ);
		ser->addTool(t);
	}
	// Save
	sirEdit::doSave();

	// Call codegen
	std::string path = filePath.substr(0, filePath.rfind("/"));
	sirEdit::runCodegen({filePath, "-t", tool->getName(), "-b"}, path);

	// Read data
	string result;
	{
		auto fileSize = ifstream(path + "/specification.skill", ios::ate | ios::binary).tellg();
		result.resize(fileSize);
		ifstream in(path + "/specification.skill", ios::binary);
		in.read(const_cast<char*>(&(result[0])), fileSize);
	}

	// Was global spec?
	if(wasGlobal) {
		ser->removeTool(const_cast<sirEdit::data::Tool*>(tool));
		sirEdit::doSave();
	}

	// Return
	return result;
}

extern void sirEdit::doSave(bool blocking) {
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
	{
		lock_guard<mutex> __lock__(saveMutex);
		ser->prepareSave();
		ser->save();
		toRunFunc();
	}
}

inline void loadFileThread(Gtk::Window* mainWindow, Gtk::Window* popup, Glib::RefPtr<Gio::File> file) {
	outputFile = file;

	// Create tmpfile
	string fileName = string(tmpnam(nullptr)) + ".sir";
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

	// Autosave
	new thread([]() -> void {
		if(ended)
			return;
		sirEdit::runInGui([]() -> void {
			sirEdit::doSave(false);
		});
		sleep(30);
	});
}

extern void sirEdit::loadFile(Gtk::Window* window, Gtk::FileChooserNative* chooser) {
	// Start loader
	auto files = chooser->get_files();
	if(files.size() != 1)
		throw; // TODO: Show error
	auto file = files[0];
	loader_thread = move(thread([window, file]() -> void { loadFileThread(window, nullptr, file); }));
}
