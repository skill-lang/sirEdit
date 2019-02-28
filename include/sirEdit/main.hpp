#pragma once
#include <functional>
#include <gtkmm.h>
#include <sirEdit/data/serialize.hpp>

namespace sirEdit {
	extern Gtk::Application* mainApplication;
	extern sirEdit::data::HistoricalView* views;

	extern void loadFile(Gtk::Window* window, Gtk::FileChooserNative* chooser);
	extern void runInGui(std::function<void()> func);
}
