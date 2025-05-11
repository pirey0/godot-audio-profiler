#ifndef AUDIO_PROFILER_PLUGIN_H
#define AUDIO_PROFILER_PLUGIN_H

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "audio_profiler_debugger_plugin.h"

using namespace godot;

class AudioProfilerPlugin : public EditorPlugin {
    GDCLASS(AudioProfilerPlugin, EditorPlugin);

private:
    Ref<AudioProfilerDebuggerPlugin> debugger;

public:
    AudioProfilerPlugin();
    ~AudioProfilerPlugin() override;

    void _notification(int p_what);

    static void _bind_methods(){

    }
};

#endif // AUDIO_PROFILER_PLUGIN_H