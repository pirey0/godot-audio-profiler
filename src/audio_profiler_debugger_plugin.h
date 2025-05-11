#ifndef AUDIO_PROFILER_DEBUGGER_PLUGIN_H
#define AUDIO_PROFILER_DEBUGGER_PLUGIN_H

#include <godot_cpp/classes/editor_debugger_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/tree.hpp>
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/line_edit.hpp"
#include "godot_cpp/classes/timer.hpp"
#include "godot_cpp/templates/hash_map.hpp"

using namespace godot;

struct AudioInfo {
	uint64_t instance_id;
	String instance_path;
	String stream_path;
	unsigned char type = 0;
	float self_volume_db = 0.0f;
	float mixed_volume_db = 0.0f;
	StringName bus;
	float playback_position = 0.0f;
	bool playing = false;
	AudioInfo() {}
};

class AudioProfilerDebuggerPlugin : public EditorDebuggerPlugin {
    GDCLASS(AudioProfilerDebuggerPlugin, EditorDebuggerPlugin);

private:
	Control* root = nullptr;
	Tree *tree = nullptr;
	Timer *refresh_timer = nullptr;
	Label *summary = nullptr;
	LineEdit *search = nullptr;
	int session_id;

	bool dirty = false;
	bool show_only_playing_nodes = true;
	bool locked = false;
	bool top10mode = false;

	Array latest_data;
	HashMap<uint64_t, AudioInfo> displayed_players;
	HashMap<uint64_t, TreeItem *> id_to_tree_item;
	HashMap<TreeItem *, uint64_t> tree_item_to_id;


public:
    static void _bind_methods(){};

    virtual void _setup_session(int32_t session_id) override;
	virtual bool _has_capture(const String &capture) const override;
	virtual bool _capture(const String &message, const Array &data, int32_t session_id) override;

	void _notification(int what);
	void on_search_changed(String new_search) { dirty = true; }
	void on_visibility_changed();
	void refresh_display();
	bool update_tree_display_of(const AudioInfo* info,  String filter);
	void on_only_active_toggled(bool value);
	void on_locked_toggled(bool value);
	void on_limit_toggled(bool value);
	void on_tree_item_activated();
	Color db_to_color(float db);

private:
    void _on_session_started();
    void _on_session_stopped();
}; 

#endif // AUDIO_PROFILER_DEBUGGER_PLUGIN_H