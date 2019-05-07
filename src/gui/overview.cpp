#include <sirEdit/data/tools.hpp>
#include <sirEdit/data/serialize.hpp>
#include <gtkmm.h>
#include <unordered_set>

#include <iostream>

using namespace sirEdit::data;

class ToolModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<bool> data_used;
		Gtk::TreeModelColumn<bool> data_active;
		Gtk::TreeModelColumn<Tool*> data_tool;

		ToolModel() {
			this->add(data_used);
			this->add(data_active);
			this->add(data_name);
			this->add(data_tool);
		}
};
static ToolModel toolModel;

class TypeModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<bool> data_used;
		Gtk::TreeModelColumn<bool> data_active;
		Gtk::TreeModelColumn<Type*> data_type;

		TypeModel() {
			this->add(data_used);
			this->add(data_active);
			this->add(data_name);
			this->add(data_type);
		}
};
static TypeModel typeModel;

class FieldModel : public Gtk::TreeModel::ColumnRecord
{
	public:
		Gtk::TreeModelColumn<Glib::ustring> data_name;
		Gtk::TreeModelColumn<bool> data_used;
		Gtk::TreeModelColumn<bool> data_active;
		Gtk::TreeModelColumn<Field*> data_type;

		FieldModel() {
			this->add(data_used);
			this->add(data_active);
			this->add(data_name);
			this->add(data_type);
		}
};
static FieldModel fieldModel;


class Overview : public Gtk::VBox {
	private:
		HistoricalView& view;

		// Stack
		Gtk::Stack stack;
		Gtk::StackSwitcher stack_switcher;

		// Type overview
		Gtk::HPaned tool_paned;
		Gtk::HPaned type_paned;
		Gtk::HPaned field_paned;

		Gtk::ToggleButton hide_inactive_tool = Gtk::ToggleButton("Hide inactive");
		Gtk::ToggleButton hide_unused_tool = Gtk::ToggleButton("Hide unused");
		Gtk::ToggleButton hide_inactive_type = Gtk::ToggleButton("Hide inactive");
		Gtk::ToggleButton hide_unused_type = Gtk::ToggleButton("Hide unused");
		Gtk::ToggleButton hide_inactive_field = Gtk::ToggleButton("Hide inactive");
		Gtk::ToggleButton hide_unused_field = Gtk::ToggleButton("Hide unused");

		Gtk::TreeView tool_view;
		Glib::RefPtr<Gtk::TreeStore> tool_store;
		Gtk::TreeView type_view;
		Glib::RefPtr<Gtk::TreeStore> type_store;
		Gtk::TreeView field_view;
		Glib::RefPtr<Gtk::TreeStore> field_store;

		//
		// Cache
		//
		std::unordered_set<const Tool*> __cache_selected_tools;
		std::unordered_set<const Tool*> __cache_used_tools;

		void genCacheTools() {
			this->__cache_selected_tools = {};
			for(auto& i : this->tool_view.get_selection()->get_selected_rows()) {
				const Tool* tmp = (*(this->tool_store->get_iter(i)))[toolModel.data_tool];
				this->__cache_selected_tools.insert(tmp);
			}
		}

		//
		// Helpers
		//
		bool blockChange = false;

		bool isToolActive(const Tool* tool) {
			// Check marked tools
			if(this->__cache_selected_tools.size() != 0) {
				bool found = false;
				for(auto& i : this->__cache_selected_tools) {
					if(i == tool)
						found = true;
				}
				if(!found)
					return false;
			}
			// TODO: Check type marked
			// TODO: Check field marked
			return true;
		}

		void updateToolData() {
			if(blockChange)
				return;
			blockChange = true;
			std::cout << "test2" << std::endl;
			this->genCacheTools();
			this->tool_view.get_selection()->unselect_all();
			this->tool_store->clear();
			for(auto& i : this->view.getStaticView().getTools()) {
				// Checks
				if(this->hide_unused_tool.property_active().get_value() && this->__cache_used_tools.find(&i) == this->__cache_used_tools.end())
					continue;
				if(this->hide_inactive_tool.property_active().get_value() && !this->isToolActive(&i))
					continue;

				// Insert
				auto tmp = *(this->tool_store->append());
				tmp[toolModel.data_name] = i.getName();
				tmp[toolModel.data_active] = this->isToolActive(&i);
				tmp[toolModel.data_used] = this->__cache_used_tools.find(&i) != this->__cache_used_tools.end();
				tmp[toolModel.data_tool] = const_cast<Tool*>(&i);
				if(this->__cache_selected_tools.find(&i) != this->__cache_selected_tools.end())
					this->tool_view.get_selection()->select(tmp);
			}
			blockChange = false;
		}

		//
		// Generators
		//
		template<class MODEL, class TOGGLE_CALLBACK>
		static void genTreeView(Gtk::TreeView& tree_view, MODEL& tree_model, const TOGGLE_CALLBACK& toggle_callback) {
			auto tmp_used = Gtk::manage(new Gtk::CellRendererToggle());
			tree_view.append_column("used", *tmp_used);
			tree_view.get_column(0)->add_attribute(tmp_used->property_active(), tree_model.data_used);
			tmp_used->signal_toggled().connect(toggle_callback);
			auto tmp_active = Gtk::manage(new Gtk::CellRendererToggle());
			tmp_active->set_activatable(false);
			tree_view.append_column("active", *tmp_active);
			tree_view.get_column(1)->add_attribute(tmp_active->property_active(), tree_model.data_active);
			tree_view.append_column("name", tree_model.data_name);
			tree_view.set_search_column(2);
			tree_view.set_expander_column(*(tree_view.get_column(2)));
			tree_view.get_selection()->set_mode(Gtk::SelectionMode::SELECTION_MULTIPLE);
		}

		//
		// Events
		//
		void toggle_tool_used(const Glib::ustring& index) {
			auto tmp = *(this->tool_store->get_iter(index));
			auto tmpInter = this->__cache_used_tools.find(static_cast<const Tool*>(tmp[toolModel.data_tool]));
			if(tmpInter == this->__cache_used_tools.end()) {
				this->__cache_used_tools.insert(static_cast<const Tool*>(tmp[toolModel.data_tool]));
				tmp[toolModel.data_used] = true;
			}
			else {
				this->__cache_used_tools.erase(tmpInter);
				tmp[toolModel.data_used] = false;
			}
			this->updateToolData();
		}
		void toggle_type_used(const Glib::ustring& index) {
			// TODO: Toogle used
		}
		void toggle_field_used(const Glib::ustring& index) {
			// TODO: Toogle used
		}

		void update_all() {
			this->updateToolData();
		}

	public:
		Overview(HistoricalView& view) : Gtk::VBox(), view(view) {
			// Init Stack
			this->stack_switcher.set_stack(this->stack);
			this->pack_start(this->stack_switcher, false, true);
			this->stack.add(this->tool_paned, "Tool Overview");
			this->pack_start(this->stack, true, true);

			// Init layout
			this->tool_paned.pack2(this->type_paned);
			{
				Gtk::VBox* tmp = Gtk::manage(new Gtk::VBox());
				tmp->pack_start(this->hide_inactive_tool, false, true);
				this->hide_inactive_tool.signal_toggled().connect([this]() -> void {
					this->updateToolData();
				});
				tmp->pack_start(this->hide_unused_tool, false, true);
				this->hide_unused_tool.signal_toggled().connect([this]() -> void {
					this->updateToolData();
				});
				tmp->pack_start(this->tool_view, true, true);
				this->tool_paned.pack1(*tmp);
			}
			this->type_paned.pack2(this->field_paned);
			{
				Gtk::VBox* tmp = Gtk::manage(new Gtk::VBox());
				tmp->pack_start(this->hide_inactive_type, false, true);
				tmp->pack_start(this->hide_unused_type, false, true);
				tmp->pack_start(this->type_view, true, true);
				this->type_paned.pack1(*tmp);
			}
			{
				Gtk::VBox* tmp = Gtk::manage(new Gtk::VBox());
				tmp->pack_start(this->hide_inactive_field, false, true);
				tmp->pack_start(this->hide_unused_field, false, true);
				tmp->pack_start(this->field_view, true, true);
				this->field_paned.pack1(*tmp);
			}
			{
				this->field_paned.pack2(*(Gtk::manage(new Gtk::Label("TODO"))));
				// TDOO: Sidebar
			}

			// Trees
			this->tool_store = Gtk::TreeStore::create(toolModel);
			this->tool_view.set_model(this->tool_store);
			genTreeView(this->tool_view, toolModel, [this](Glib::ustring index) -> void {
				this->toggle_tool_used(index);
			});
			this->tool_view.get_selection()->signal_changed().connect([this]() -> void {
				this->update_all();
			});

			this->type_store = Gtk::TreeStore::create(typeModel);
			this->type_view.set_model(this->type_store);
			genTreeView(this->type_view, typeModel, [this](Glib::ustring index) -> void {
				this->toggle_type_used(index);
			});

			this->field_store = Gtk::TreeStore::create(fieldModel);
			this->field_view.set_model(this->field_store);
			genTreeView(this->field_view, fieldModel, [this](Glib::ustring index) -> void {
				this->toggle_field_used(index);
			});

			// Content
			this->update_all();

			// Change notification
			this->view.addChangeCallback([this]() -> void {
				this->update_all();
			});
		}
};

namespace sirEdit::gui {
	Gtk::Widget* createOverview(HistoricalView& historicalView) {
		return new Overview(historicalView);
	}
}
