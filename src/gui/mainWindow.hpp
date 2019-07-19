#pragma once
#include <sirEdit/data/serialize.hpp>
#include <gtkmm.h>

namespace sirEdit::gui {
	/**
	 * Open the main window
	 * @param serializer the serializer to to use
	 * @param file the output file
	 */
	extern void openMainWindow(std::unique_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file);
	/**
	 * Create tool to edit widget
	 * @param name the name of the tool
	 * @parma historicalView the history
	 * @return the widget that allows to edit the tool
	 */
	extern Gtk::Widget* createToolEdit(std::string name,sirEdit::data::Transactions& historicalView);
	/**
	 * Create a overview of all tools
	 * @param historicalView the history
	 * @return the widget that allow to get an overview
	 */
	extern Gtk::Widget* createOverview(sirEdit::data::Transactions& historicalView);
}
