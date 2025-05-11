#ifndef AUDIO_PROFILER_H
#define AUDIO_PROFILER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/classes/audio_stream_player2d.hpp>
#include <godot_cpp/classes/audio_stream_player3d.hpp>

using namespace godot;


class AudioProfiler : public Object {

private:
    static AudioProfiler* singleton;

    //variable, controls periodicity of updates
    float refresh_time = 0.5;

    HashSet<uint64_t> active_players;
    float time_to_refresh = 0.0;
    bool active = false;

    void on_node_added(Node* node);
    void on_node_removed(Node* node);
    void on_physics_frame();
    void update_debugger();
    bool on_debugger_message_received(String msg, Array data);
    void set_active(bool active);
    void collect_initial_set();
    
    HashMap<StringName, float> create_bus_chain_loudness_map();
    Array find_all_viewports();
    float get_compound_bus_volume(const String &starting_bus);
    float estimate_mixed_volume(Node* node, float volume_db, StringName bus, HashMap<StringName, float>& bus_cache, char node_type, Array viewports);
    float estimate_attenuation_for(Node* node, char node_type, Array viewports);
    float estimate_attenuation_for(AudioStreamPlayer2D* player, Array viewports);
    float estimate_attenuation_for(AudioStreamPlayer3D* player, Array viewports);
    static char node_to_player_type(Node* node);
    
    public:
    AudioProfiler();
    ~AudioProfiler();
    
    static void initialize();
    static void deinitialize();
};

#endif  // AUDIO_PROFILER_H