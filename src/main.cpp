#include <atomic>
#include <gtkmm.h>
#include <File.h>
#include <functional>
#include <iostream>
#include <list>

#include "gui/startupWindow.hpp"

#include <sirEdit/data/serialize.hpp>

using namespace sir::api;
using namespace std;

namespace sirEdit {
	Gtk::Application* mainApplication;
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

volatile bool ended = false;

int main(int args, char** argv) {
	// GIO application
	auto application = Gtk::Application::create("de.marko10-000.sirEdit");
	sirEdit::mainApplication = application.get();
	application->signal_activate().connect([]() -> void {
		// Dispacher func for function call
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

		// Open main dialog
		sirEdit::gui::runStartupWindow();
	});

	// Run and terminate
	auto result = application->run(args, argv);
	ended = true;
	return result;
}
