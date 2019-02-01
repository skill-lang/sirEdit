#pragma once
#include <sirEdit/data/serialize.hpp>
#include <gtkmm.h>

namespace sirEdit::gui {
	extern void openMainWindow(std::shared_ptr<sirEdit::data::Serializer> serializer, Glib::RefPtr<Gio::File> file);
}
