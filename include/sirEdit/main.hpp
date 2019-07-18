#pragma once
#include <functional>
#include <gtkmm.h>
#include <sirEdit/data/serialize.hpp>
#include <string>
#include <vector>


namespace sirEdit {
	extern Gtk::Application* mainApplication;

	/**
	 * Save the file
	 * @param blocking should block till it's done
	 */
	extern void doSave(bool blocking=false);
	/**
	 * Returns path where it's saved (remotly)
	 * @param saved path
	 */
	extern std::string getSavePath();
	/**
	 * Returns the current sir file path
	 * @return sir saved path
	 */
	extern std::string getSirPath();
	/**
	 * Gnerates a tool spec
	 * @param tool the toolspec that should be used. When empty global sepc
	 * @return The serialized skill spec
	 */
	extern std::string getSpec(const sirEdit::data::Tool* tool);

	/**
	 * Load a file and then close the window
	 * @param window the window to close
	 * @param chooser the source of the data
	 */
	extern void loadFile(Gtk::Window* window, Gtk::FileChooserNative* chooser);
	/**
	 * Run an lambda in the gui thread
	 * @param func the function that should run
	 */
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
