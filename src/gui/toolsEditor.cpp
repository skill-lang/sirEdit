#include "mainWindow.hpp"
#include <gtkmm.h>

#include <iostream>

//
// Model for type  list
//
class TypeListModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<size_t> data_id;
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<bool> data_status_r;
		Gtk::TreeModelColumn<bool> data_status_w;
		Gtk::TreeModelColumn<bool> data_status_d;
		Gtk::TreeModelColumn<bool> data_status_u;
		Gtk::TreeModelColumn<bool> data_status_no;

		TypeListModel() {
			this->add(data_id);
			this->add(data_status_no);
			this->add(data_status_u);
			this->add(data_status_r);
			this->add(data_status_w);
			this->add(data_status_d);
			this->add(data_name);
		}
};
static TypeListModel typeListModel;
static Glib::RefPtr<Gtk::TreeStore> typeListData;

//
// Field tree
//
class FieldListModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<bool> data_status_r;
		Gtk::TreeModelColumn<bool> data_status_w;
		Gtk::TreeModelColumn<bool> data_status_e;
		Gtk::TreeModelColumn<bool> data_status_u;
		Gtk::TreeModelColumn<bool> data_status_no;

		FieldListModel() {
			this->add(data_name);
			this->add(data_status_no);
			this->add(data_status_u);
			this->add(data_status_r);
			this->add(data_status_w);
			this->add(data_status_e);
		}
};
static FieldListModel fieldListModel;
static Glib::RefPtr<Gtk::TreeStore> fieldListData;

inline void buildTreeSubRows(Gtk::TreeStore::Row& row, const sirEdit::data::Type* super) {
	for(auto& i : super->getSubTypes()) {
		Gtk::TreeStore::Row tmp = *(typeListData->append(row.children()));
		tmp[typeListModel.data_id] = i->getID();
		tmp[typeListModel.data_name] = i->getName();
		buildTreeSubRows(tmp, i);
	}
}

class Chooser : public Gtk::CellRenderer {
	private:
		Gtk::ButtonBox widget;
		Gtk::OffscreenWindow ow;

		template<class T>
		Gtk::Button* newButton(T name) {
			Gtk::Button* button = new Gtk::ToggleButton();
			button->add(*(new Gtk::Label(name)));
			return button;
		}

	public:
		Chooser() {
			this->widget.add(*(newButton("R")));
			this->widget.add(*(newButton("W")));
			this->widget.add(*(newButton("C")));
			this->widget.add(*(newButton("U")));
			this->widget.add(*(newButton("-")));
			this->widget.get_style_context()->add_class("linked");
			this->ow.add(this->widget);
			this->ow.show_all();
		}

		Gtk::SizeRequestMode get_request_mode_vfunc() const {
			return this->widget.get_request_mode();
		}
		void get_preferred_width_vfunc(Gtk::Widget& widget, int& minimum_width, int& natural_width) const {
			int tmp;
			this->widget.get_preferred_width(minimum_width, tmp);
			natural_width = minimum_width;
		}
		void get_preferred_height_for_width_vfunc(Gtk::Widget& widget, int width, int& minimum_height, int& natural_height) const {
			this->widget.get_preferred_height_for_width(width, minimum_height, natural_height);
		}
		void get_preferred_height_vfunc(Gtk::Widget& widget, int& minimum_height, int& natural_height) const {
			this->widget.get_preferred_height(minimum_height, natural_height);
		}
		void get_preferred_width_for_height_vfunc(Gtk::Widget& widget, int height, int& minimum_width, int& natural_width) const {
			this->widget.get_preferred_width_for_height(height, minimum_width, natural_width);
		}
		void render_vfunc(const ::Cairo::RefPtr< ::Cairo::Context>& cr, Gtk::Widget& widget, const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area, Gtk::CellRendererState flags) {
			this->ow.set_size_request(cell_area.get_width(), cell_area.get_height());
			Gdk::Cairo::set_source_pixbuf(cr, this->ow.get_pixbuf(), cell_area.get_x(), cell_area.get_y());
			cr->paint();
		}
};

//
// Show widget
//
extern Gtk::Widget* sirEdit::gui::createToolEdit(std::string name, const sirEdit::data::View view) {
	// Generate panes
	Gtk::Paned* paned = new Gtk::HPaned();
	Gtk::ScrolledWindow* scrollable_left = new Gtk::ScrolledWindow();
	scrollable_left->set_size_request(300, 200);
	paned->pack1(*scrollable_left, false, false);
	Gtk::ScrolledWindow* scrollable_right = new Gtk::ScrolledWindow();
	paned->pack2(*scrollable_right, true, false);

	// Add tree views
	Gtk::TreeView* view_left = new Gtk::TreeView();
	scrollable_left->add(*view_left);
	Gtk::TreeView* view_right  = new Gtk::TreeView();
	scrollable_right->add(*view_right);

	// Types and field lists
	{
		typeListData = Gtk::TreeStore::create(typeListModel);
		typeListData->set_sort_column(typeListModel.data_name, Gtk::SortType::SORT_ASCENDING);
		if(!typeListData)
			throw;

		{
			for(auto& i : view.getBaseTypes()) {
				Gtk::TreeStore::Row tmp = *(typeListData->append());
				tmp[typeListModel.data_id] = i->getID();
				tmp[typeListModel.data_name] = i->getName();
				buildTreeSubRows(tmp, i);
			}
		}

		view_left->set_model(typeListData);
		view_left->set_search_column(typeListModel.data_name);
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_left->append_column("-", *tmp);
			view_left->get_column(0)->add_attribute(tmp->property_active(), typeListModel.data_status_no);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_left->append_column("u", *tmp);
			view_left->get_column(1)->add_attribute(tmp->property_active(), typeListModel.data_status_u);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_left->append_column("r", *tmp);
			view_left->get_column(2)->add_attribute(tmp->property_active(), typeListModel.data_status_r);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_left->append_column("w", *tmp);
			view_left->get_column(3)->add_attribute(tmp->property_active(), typeListModel.data_status_w);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_left->append_column("d", *tmp);
			view_left->get_column(4)->add_attribute(tmp->property_active(), typeListModel.data_status_d);
		}
		view_left->append_column("Types", typeListModel.data_name);
		auto nameColumn = view_left->get_column(5);
		view_left->set_activate_on_single_click(true);
		view_left->expand_all();
		view_left->set_expander_column(*nameColumn);

		fieldListData = Gtk::TreeStore::create(fieldListModel);
		fieldListData->set_sort_column(fieldListModel.data_name, Gtk::SortType::SORT_ASCENDING);
		view_right->set_model(fieldListData);
		view_right->set_search_column(fieldListModel.data_name);
		//Chooser* cl = new Chooser();
		//view_right->append_column("Status", *cl);
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_right->append_column("-", *tmp);
			view_right->get_column(0)->add_attribute(tmp->property_active(), typeListModel.data_status_no);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_right->append_column("u", *tmp);
			view_right->get_column(1)->add_attribute(tmp->property_active(), fieldListModel.data_status_u);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_right->append_column("r", *tmp);
			view_right->get_column(2)->add_attribute(tmp->property_active(), fieldListModel.data_status_r);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_right->append_column("w", *tmp);
			view_right->get_column(3)->add_attribute(tmp->property_active(), fieldListModel.data_status_w);
		}
		{
			auto tmp = new Gtk::CellRendererToggle();
			tmp->signal_toggled().connect([](Glib::ustring test) -> void {
				std::cout << test << std::endl;
			});
			tmp->set_property("radio", true);
			view_right->append_column("e", *tmp);
			view_right->get_column(4)->add_attribute(tmp->property_active(), fieldListModel.data_status_e);
		}
		view_right->append_column("Name", fieldListModel.data_name);
		view_right->set_expander_column(*(view_right->get_column(5)));

		view_left->signal_row_activated().connect([view](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) -> void {
			Gtk::TreeModel::iterator iter = typeListData->get_iter(path);
			if(iter) {
				Gtk::TreeModel::Row row = *iter;
				fieldListData->clear();
				sirEdit::data::TypeWithFields* type = static_cast<sirEdit::data::TypeWithFields*>(view.getTypes()[row[typeListModel.data_id]].get());
				for(auto& i: type->getFields()) {
					Gtk::TreeStore::Row tmp = *(fieldListData->append());
					tmp[fieldListModel.data_name] = i.getName();
					tmp[fieldListModel.data_status_r] = false;
					tmp[fieldListModel.data_status_w] = false;
					tmp[fieldListModel.data_status_e] = false;
					tmp[fieldListModel.data_status_u] = false;
					tmp[fieldListModel.data_status_no] = true;
				}
			}
		});
	}

	// Return paned
	paned->show_all();
	return paned;
}
