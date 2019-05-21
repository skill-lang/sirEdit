#pragma once
#include <sirEdit/data/serialize.hpp>
#include <gtkmm.h>

namespace sirEdit::gui {
	extern void openMainWindow(std::unique_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file);
	extern Gtk::Widget* createToolEdit(std::string name,sirEdit::data::Transactions& historicalView);
	extern Gtk::Widget* createOverview(sirEdit::data::Transactions& historicalView);
}
