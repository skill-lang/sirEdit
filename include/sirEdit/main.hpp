#pragma once
#include <functional>
#include <gtkmm.h>

namespace sirEdit {
	extern Gtk::Application* mainApplication;

	extern void loadFile(Gtk::Window* window, Gtk::FileChooserNative* chooser);
	extern void runInGui(std::function<void()> func);
}
