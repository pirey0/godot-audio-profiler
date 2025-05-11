#include "audio_profiler.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/audio_listener2d.hpp>
#include <godot_cpp/classes/audio_listener3d.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>

AudioProfiler* AudioProfiler::singleton = nullptr;

void AudioProfiler::initialize(){
    memnew(AudioProfiler);
}

void AudioProfiler::deinitialize(){
    if (singleton) {
		memdelete(singleton);
	}
}

AudioProfiler::AudioProfiler() {
    singleton = this;
    EngineDebugger::get_singleton()->register_message_capture("audio_profiler_display", callable_mp(this,&AudioProfiler::on_debugger_message_received));

    EngineDebugger::get_singleton()->send_message("audio_profiler:tracker_created", Array());
}

AudioProfiler::~AudioProfiler() {
    set_active(false);
    if (EngineDebugger::get_singleton()){
        EngineDebugger::get_singleton()->unregister_message_capture("audio_profiler_display");
        EngineDebugger::get_singleton()->send_message("audio_profiler:tracker_removed", Array());
    }
}

void AudioProfiler::set_active(bool active){
    if (this->active == active){
        return;
    }
    this->active = active;

    //UtilityFunctions::print("set_active: ", active);
    SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    if(active){
        collect_initial_set();
        tree->connect("node_added", callable_mp(singleton, &AudioProfiler::on_node_added));
        tree->connect("node_removed", callable_mp(singleton, &AudioProfiler::on_node_removed));
        tree->connect("physics_frame", callable_mp(singleton, &AudioProfiler::on_physics_frame));
        time_to_refresh = refresh_time;
    
    } else {
        active_players.clear();
        if (tree){
            tree->disconnect("node_added", callable_mp(singleton, &AudioProfiler::on_node_added));
            tree->disconnect("node_removed", callable_mp(singleton, &AudioProfiler::on_node_removed));
            tree->disconnect("physics_frame", callable_mp(singleton, &AudioProfiler::on_physics_frame));
        }
    }
}

void AudioProfiler::collect_initial_set()
{
    active_players.clear();

    SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node *root = tree->get_root();
    // Find all AudioStreamPlayer nodes (2D, 3D, generic)
    Array players = root->find_children("*", "AudioStreamPlayer", true, false);
    Array players2d = root->find_children("*", "AudioStreamPlayer2D", true, false);
    Array players3d = root->find_children("*", "AudioStreamPlayer3D", true, false);
    for (int i = 0; i < players.size(); i++) {
        singleton->on_node_added(Object::cast_to<Node>(players[i]));
    }
    for (int i = 0; i < players2d.size(); i++) {
        singleton->on_node_added(Object::cast_to<Node>(players2d[i]));
    }
    for (int i = 0; i < players3d.size(); i++) {
        singleton->on_node_added(Object::cast_to<Node>(players3d[i]));
    }

}

HashMap<StringName, float> AudioProfiler::create_bus_chain_loudness_map()
{
    auto out = HashMap<StringName, float>();
    AudioServer *server = AudioServer::get_singleton();

    for (size_t i = 0; i < server->get_bus_count(); i++)
    {
        StringName name = server->get_bus_name(i);
        out[name] = get_compound_bus_volume(name);
    }

    return out;
}

Array AudioProfiler::find_all_viewports() {
    Array viewports;
    
    SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node* root = tree->get_root();
    if (!root) {
        return viewports;
    }

    Vector<Node*> stack;
    stack.push_back(root);

    while (!stack.is_empty()) {
        Node* current = stack[stack.size() - 1];
        stack.resize(stack.size() - 1); // Pop

        if (Viewport* vp = Object::cast_to<Viewport>(current)) {
            viewports.push_back(vp);
        }

        const int child_count = current->get_child_count();
        for (int i = child_count - 1; i >= 0; --i) { // Add children in reverse to preserve order
            Node* child = current->get_child(i);
            if (child) {
                stack.push_back(child);
            }
        }
    }

    return viewports;
}

float AudioProfiler::get_compound_bus_volume(const String &starting_bus) {

    float total_linear = 1.0f;
    String current = starting_bus;
    AudioServer *server = AudioServer::get_singleton();

    while (true) {
        int bus_index = server->get_bus_index(current);
        if (bus_index == -1) break;

        float db = server->get_bus_volume_db(bus_index);
        total_linear *= UtilityFunctions::db_to_linear(db);

        current = server->get_bus_send(bus_index);
    }

    return total_linear;
}

float AudioProfiler::estimate_mixed_volume(Node *node, float volume_db, StringName bus, HashMap<StringName, float> &bus_cache, char node_type, Array viewports)
{
    float stream_volume = UtilityFunctions::db_to_linear(volume_db);
    float bus_volume = bus_cache[bus];
    float attenuation_volume = estimate_attenuation_for(node, node_type, viewports);
    float linear = stream_volume * bus_volume * attenuation_volume;
    return UtilityFunctions::linear_to_db(linear);
}

float AudioProfiler::estimate_attenuation_for(Node *node, char node_type, Array viewports)
{
    switch (node_type)
    {
        case 2:
            return estimate_attenuation_for(static_cast<AudioStreamPlayer2D*>(node),viewports);
        case 3:
            return estimate_attenuation_for(static_cast<AudioStreamPlayer3D*>(node), viewports);
    }
    return 1.0;
}

float AudioProfiler::estimate_attenuation_for(AudioStreamPlayer2D *player, Array viewports)
{
    //This is the mirror of the internal workings of AudioStreamPlayer2D
    //Ideally this would somehow be expopsed and we would access it directly

    Ref<World2D> world_2d = player->get_world_2d();
    if (world_2d.is_null())
        return 1.0;

    Vector2 global_pos = player->get_global_position();
    StringName actual_bus = player->get_bus();
    float max_multiplyer = 0.0;
    for (size_t i = 0; i < viewports.size(); i++)
    {
        Viewport* vp = Object::cast_to<Viewport>(viewports[i].operator godot::Object *());
        
        if (!vp->is_audio_listener_2d()) {
            continue;
        }
        //compute matrix to convert to screen
        Vector2 screen_size = vp->get_visible_rect().size;
        Vector2 listener_in_global;
        Vector2 relative_to_listener;

        //screen in global is used for attenuation
        AudioListener2D *listener = vp->get_audio_listener_2d();
        Transform2D full_canvas_transform = vp->get_global_canvas_transform() * vp->get_canvas_transform();
        if (listener) {
            listener_in_global = listener->get_global_position();
            relative_to_listener = (global_pos - listener_in_global).rotated(-listener->get_global_rotation());
            relative_to_listener *= full_canvas_transform.get_scale(); // Default listener scales with canvas size, do the same here.
        } else {
            listener_in_global = full_canvas_transform.affine_inverse().xform(screen_size * 0.5);
            relative_to_listener = full_canvas_transform.xform(global_pos) - screen_size * 0.5;
        }

        float dist = global_pos.distance_to(listener_in_global); // Distance to listener, or screen if none.
        float max_distance = player->get_max_distance();
        float attenuation = player->get_attenuation();
        if (dist > max_distance) {
            continue; // Can't hear this sound in this viewport.
        }

        float multiplier = Math::pow(1.0f - dist / max_distance, attenuation);
        max_multiplyer = MAX(multiplier, max_multiplyer);
    }

        return max_multiplyer;
}

float _get_attenuation_db(AudioStreamPlayer3D* src,float p_distance) {
	float att = 0;
	switch (src->get_attenuation_model()) {
		case AudioStreamPlayer3D::ATTENUATION_INVERSE_DISTANCE: {
			att = UtilityFunctions::linear_to_db(1.0 / ((p_distance / src->get_unit_size()) + CMP_EPSILON));
		} break;
		case AudioStreamPlayer3D::ATTENUATION_INVERSE_SQUARE_DISTANCE: {
			float d = (p_distance / src->get_unit_size());
			d *= d;
			att = UtilityFunctions::linear_to_db(1.0 / (d + CMP_EPSILON));
		} break;
		case AudioStreamPlayer3D::ATTENUATION_LOGARITHMIC: {
			att = -20 * Math::log(p_distance / src->get_unit_size() + CMP_EPSILON);
		} break;
		case AudioStreamPlayer3D::ATTENUATION_DISABLED:
			break;
		default: {
			ERR_PRINT("Unknown attenuation type");
			break;
		}
	}

	att += src->get_volume_db();
	if (att > src->get_max_db()) {
		att = src->get_max_db();
	}

	return att;
}

float AudioProfiler::estimate_attenuation_for(AudioStreamPlayer3D *player, Array viewports)
{
    //This is the mirror of the internal workings of AudioStreamPlayer3D
    //Ideally this would somehow be expopsed and we would access it directly

        Vector3 global_pos = player->get_global_transform().origin;
        Ref<World3D> world_3d = player->get_world_3d();
        ERR_FAIL_COND_V(world_3d.is_null(), 0.0f);
    
        HashSet<Camera3D *> cameras; //=  world_3d->get_cameras();
        cameras.insert(player->get_viewport()->get_camera_3d());
    
        for (Camera3D *camera : cameras) {
            if (!camera) {
                continue;
            }
            Viewport *vp = camera->get_viewport();
            if (!vp || !vp->is_audio_listener_3d()) {
                continue;
            }
    
            Node3D *listener_node = camera;
            if (AudioListener3D *listener = vp->get_audio_listener_3d()) {
                listener_node = listener;
            }
    
            Vector3 local_pos = listener_node->get_global_transform().orthonormalized().affine_inverse().xform(global_pos);
            float dist = local_pos.length();
    
            auto max_distance = player->get_max_distance();
            if (max_distance > 0 && dist > max_distance) {
                continue; // Listener too far
            }
    
            float multiplier = UtilityFunctions::db_to_linear(_get_attenuation_db(player,dist));
            if (max_distance > 0) {
                multiplier *= MAX(0.0f, 1.0f - (dist / max_distance));
            }
    
            float db_att = (1.0f - MIN(1.0f, multiplier)) * player->get_attenuation_filter_db();
    
            if (player->is_emission_angle_enabled()) {
                Vector3 listenertopos = global_pos - listener_node->get_global_transform().origin;
                float c = listenertopos.normalized().dot(player->get_global_transform().basis.get_column(2).normalized());
                float angle = Math::rad_to_deg(Math::acos(c));
                if (angle > player->get_emission_angle()) {
                    db_att -= - player->get_emission_angle_filter_attenuation_db();
                }
            }
    
            return UtilityFunctions::db_to_linear(db_att);
        }
        return 0.0;
}

void AudioProfiler::on_node_added(Node* node){
    
    if (AudioProfiler::node_to_player_type(node) != 0){
        active_players.insert(node->get_instance_id());
    }
}

void AudioProfiler::on_node_removed(Node *node)
{
    uint64_t id = node->get_instance_id();
    active_players.erase(id);
}

void AudioProfiler::on_physics_frame()
{
    if (!EngineDebugger::get_singleton()->is_active())
        return;

    Engine::get_singleton()->get_physics_ticks_per_second();
    float physics_delta = 1.0f / Engine::get_singleton()->get_physics_ticks_per_second(); 
    time_to_refresh -= physics_delta;

    if (time_to_refresh < 0.0f){
        time_to_refresh += refresh_time;
        update_debugger();
    }

}

void AudioProfiler::update_debugger() {
    if (!EngineDebugger::get_singleton()->is_active()) 
        return;

    Array output;

    auto bus_loudness_map = create_bus_chain_loudness_map();
    auto viewports = find_all_viewports();

    for (uint64_t id : active_players)
    {
        Node* node = cast_to<Node>(ObjectDB::get_instance(id));
        if (node == nullptr){
            UtilityFunctions::printerr("Invalid stream player cached: ", id);
            continue;
        }
        Array entry;
        entry.append(id); //0
        entry.append(node->get_path()); //1

        bool playing = node->get("playing");
        entry.append(playing); //2
        char player_type = node_to_player_type(node);
        entry.append(player_type); //3

        if (playing){

            Ref<AudioStream> stream = static_cast<Ref<AudioStream>>(node->call("get_stream"));
            if (stream.is_valid()){
                entry.append(String(stream.ptr()->get_path())); //4
            }else{
                entry.append(""); //4
            }
            
            float volume_db = node->get("volume_db");
            StringName bus = node->get("bus"); 

            
            entry.append(volume_db); //5
            entry.append(estimate_mixed_volume(node, volume_db, bus, bus_loudness_map, player_type, viewports)); //6
            entry.append(bus); //7
            entry.append(node->call("get_playback_position")); //8

        }
        
        output.append(entry);

    }
    
    EngineDebugger::get_singleton()->send_message("audio_profiler:update", output);
}

bool AudioProfiler::on_debugger_message_received(String msg, Array data)
{
    if (msg == "active"){
        bool active = data[0];
        set_active(active);
        return true;
    }
    return false;
}

char AudioProfiler::node_to_player_type(Node *node)
{
    if(node->is_class("AudioStreamPlayer"))
        return 1;

    if(node->is_class("AudioStreamPlayer2D"))
        return 2;

    if(node->is_class("AudioStreamPlayer3D"))
        return 3;     

    return 0;
}
