#pragma once
#include <functional>
#include <gtkmm.h>
#include <sirEdit/data/serialize.hpp>
#include <string>
#include <vector>


namespace sirEdit {
	extern Gtk::Application* mainApplication;

	extern void doSave(bool blocking=false);

	extern void loadFile(Gtk::Window* window, Gtk::FileChooserNative* chooser);
	extern void runInGui(std::function<void()> func);
	/**
	 * Run a codegenerator in a directory. It will export the codegenerator automatically
	 * @param arguments the argument that should be send to the codegenerator
	 * @param path the path in witch the codegenerator should be run
	 * @return status code
	 */
	extern int runCodegen(const std::vector<std::string>& arguments, const std::string& path);
	/**
	 * Run a command
	 * @param arguments the programm and arguments that should be used
	 * @param path the path in that the programm will be executed
	 * @param status code
	 */
	extern int runCommand(const std::vector<std::string>& arguments, const std::string& path);
}
