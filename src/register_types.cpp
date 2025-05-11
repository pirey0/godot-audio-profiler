#include "register_types.h"
#include <godot_cpp/core/class_db.hpp>
#include "godot_cpp/classes/editor_plugin.hpp"
#include "audio_profiler_plugin.h"
#include "audio_profiler_debugger_plugin.h"
#include "audio_profiler.h"
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void initialize_extension(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE){
        // Only initialize proflier in non-editor builds
        if (!Engine::get_singleton()->is_editor_hint()) {
            AudioProfiler::initialize();
        }
    }

    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        GDREGISTER_CLASS(AudioProfilerPlugin);
        GDREGISTER_CLASS(AudioProfilerDebuggerPlugin);

        EditorPlugins::add_by_type<AudioProfilerPlugin>();
    }
}

void uninitialize_extension(ModuleInitializationLevel p_level) {
    AudioProfiler::deinitialize();
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_extension);
	init_obj.register_terminator(uninitialize_extension);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}