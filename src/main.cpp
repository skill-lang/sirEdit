#include <atomic>
#include <gtkmm.h>
#include <File.h>
#include <functional>
#include <iostream>
#include <list>

#include "classInfo.hpp"

#include "gui/startupWindow.hpp"

#include <sirEdit/data/serialize.hpp>

using namespace sir::api;
using namespace std;

namespace sirEdit {
	Gtk::Application* mainApplication;
	sirEdit::data::HistoricalView* views = nullptr;
	static Glib::Dispatcher* dispatcher;
	static list<std::function<void()>> dispatcher_funcs;
	static mutex dispatcher_mutex;

	void runInGui(std::function<void()> func) {
		// Add func
		{
			lock_guard<mutex> _guard(dispatcher_mutex);
			dispatcher_funcs.push_back(func);
		}

		// Notify
		dispatcher->emit();
	}
}

int main(int args, char** argv) {
	// Test Sir
	/*SkillFile* file = SkillFile::read("skill.sir");
	auto _tmp = file->UserdefinedType->allObjects();
	auto& tmp = *_tmp;
	while(tmp.hasNext()) {
		cout << getName(*static_cast<sir::UserdefinedType*>(&(*tmp))) << endl;
		++tmp;
	}*/

	// GIO application
	auto application = Gtk::Application::create("de.marko10-000.sirEdit");
	sirEdit::mainApplication = application.get();
	application->signal_activate().connect([]() -> void {
		sirEdit::dispatcher = new Glib::Dispatcher();
		sirEdit::dispatcher->connect([]() -> void {
			// Get func
			std::function<void()> func;
			{
				lock_guard<mutex> _guard(sirEdit::dispatcher_mutex);
				if(sirEdit::dispatcher_funcs.size() == 0)
					throw; // TODO: Exception
				func = *(sirEdit::dispatcher_funcs.begin());
				sirEdit::dispatcher_funcs.pop_front();
			}

			// Run func
			func();
		});
		sirEdit::gui::runStartupWindow();
	});
	auto reuslt = application->run(args, argv);
	if(sirEdit::views != nullptr)
		delete sirEdit::views;
	return reuslt;

	// GTK startup
	//Gtk::Main gtkMain(args, argv);
	//Gtk::Window window;
	//window.show_all();
	//return gtkMain.run(window);
}
