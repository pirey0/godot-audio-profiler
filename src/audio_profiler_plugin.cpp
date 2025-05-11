#include "audio_profiler_plugin.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/utility_functions.hpp>


AudioProfilerPlugin::AudioProfilerPlugin() {
    debugger.instantiate();
}

AudioProfilerPlugin::~AudioProfilerPlugin() {
    // Cleanup code if needed
}

void AudioProfilerPlugin::_notification(int p_what)
{
    switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			add_debugger_plugin(debugger);
		break;
		case NOTIFICATION_EXIT_TREE: 
			remove_debugger_plugin(debugger);
        break;
	}
}