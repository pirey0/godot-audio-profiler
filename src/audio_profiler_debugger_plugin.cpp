#include "audio_profiler_debugger_plugin.h"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/classes/editor_debugger_session.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godot_cpp/classes/v_box_container.hpp"
#include "godot_cpp/classes/h_box_container.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/line_edit.hpp"
#include "godot_cpp/classes/timer.hpp"
#include "godot_cpp/classes/file_system_dock.hpp"
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/classes/translation_server.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include "godot_cpp/templates/hash_set.hpp"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/color.hpp>
#include <algorithm>

#define TTR(text) TranslationServer::get_singleton()->translate(text)

using namespace godot;

    void AudioProfilerDebuggerPlugin::_on_session_started() {
		latest_data = Array();
		dirty = true;
    }

    void AudioProfilerDebuggerPlugin::_on_session_stopped() {
		latest_data = Array();
		dirty = true;
		refresh_display();
    }

    bool AudioProfilerDebuggerPlugin::_has_capture(const String &capture) const
    {
        return capture == "audio_profiler";
    }

    bool AudioProfilerDebuggerPlugin::_capture(const String &message, const Array &data, int32_t session_id)
    {
		if(message == "audio_profiler:tracker_created"){
			//when the tracker is created, we notify it to activate if we are already active
			bool active = root ? root->is_visible_in_tree() : false;
			if (active){
				Array data;
				data.append(active);
				get_session(session_id).ptr()->send_message("audio_profiler_display:active", data);
			}
		}else if(message == "audio_profiler:tracker_removed"){
			latest_data = Array();
			dirty = true;
			return true;
		} else if (message == "audio_profiler:update") {
			latest_data = data;
			dirty = true;
            return true;
        }

        return false;
    }


void AudioProfilerDebuggerPlugin::_setup_session(int p_session_id) {
	Ref<EditorDebuggerSession> session = get_session(p_session_id);
	ERR_FAIL_COND(session.is_null());
	session_id = p_session_id;

	VBoxContainer *root = memnew(VBoxContainer);
	root->set_name(TTR("Audio"));
	root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	this->root = root;
	root->connect("visibility_changed", callable_mp(this, &AudioProfilerDebuggerPlugin::on_visibility_changed));

	HBoxContainer *hbox = memnew(HBoxContainer);
	root->add_child(hbox);

	summary = memnew(godot::Label);
	summary->set_custom_minimum_size(Size2(200, 0));
	hbox->add_child(summary);

	search = memnew(godot::LineEdit);
	search->set_custom_minimum_size(Size2(150, 0));
	search->set_placeholder(TTR("Search"));
	search->connect("text_changed", callable_mp(this, &AudioProfilerDebuggerPlugin::on_search_changed));
    search->set_right_icon(search->get_theme_icon(StringName("Search"), StringName("EditorIcons")));
	hbox->add_child(search);

	CheckBox* only_active = memnew(godot::CheckBox);
	only_active->set_text(TTR("Active Only"));
	only_active->set_tooltip_text(TTR("Display only players that are actively emitting sound."));
	only_active->set_pressed_no_signal(true);
	only_active->connect("toggled",  callable_mp(this,&AudioProfilerDebuggerPlugin::on_only_active_toggled));
	hbox->add_child(only_active);

	
	CheckBox* locked_check = memnew(godot::CheckBox);
	locked_check->set_text(TTR("Lock"));
	locked_check->set_tooltip_text(TTR("Lock the current view to facilitate inspection."));
	locked_check->connect("toggled",  callable_mp(this,&AudioProfilerDebuggerPlugin::on_locked_toggled));
	hbox->add_child(locked_check);


	CheckBox* limit_check = memnew(godot::CheckBox);
	limit_check->set_text(TTR("Loudest Only"));
	limit_check->set_tooltip_text(TTR("Limit the view to only the 10 loudest sources."));
	limit_check->connect("toggled",  callable_mp(this,&AudioProfilerDebuggerPlugin::on_limit_toggled));
	hbox->add_child(limit_check);

	tree = memnew(Tree);
	tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tree->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root->add_child(tree);
	tree->set_hide_root(true);
	tree->create_item();
	tree->set_columns(8);
	tree->connect("item_activated", callable_mp(this, &AudioProfilerDebuggerPlugin::on_tree_item_activated));

	tree->set_column_titles_visible(true);
	tree->set_column_title(0, TTR("Source"));
	tree->set_column_title(1, TTR("Stream"));
	tree->set_column_title(2, TTR("On"));
	tree->set_column_custom_minimum_width(2, 30);
	tree->set_column_expand(2,false);
	tree->set_column_title(3, TTR("Type"));
	tree->set_column_custom_minimum_width(3, 50);
	tree->set_column_expand(3,false);
	tree->set_column_title(4, TTR("Bus"));
	tree->set_column_custom_minimum_width(4, 100);
	tree->set_column_expand(4,false);
	tree->set_column_title(5, TTR("Mixed Volume"));
	tree->set_column_custom_minimum_width(5, 80);
	tree->set_column_expand(5,false);
	tree->set_column_title(6, TTR("Volume"));
	tree->set_column_custom_minimum_width(6, 80);
	tree->set_column_expand(6,false);
	tree->set_column_title(7, TTR("Playback Time"));
	tree->set_column_custom_minimum_width(7, 80);
	tree->set_column_expand(7,false);

	refresh_timer = memnew(godot::Timer);
	refresh_timer->set_wait_time(0.1);
	refresh_timer->connect("timeout", callable_mp(this, &AudioProfilerDebuggerPlugin::refresh_display));
	refresh_timer->set_autostart(false);
	root->add_child(refresh_timer);

	session->add_session_tab(root);
}

void AudioProfilerDebuggerPlugin::on_visibility_changed()
{
	Ref<EditorDebuggerSession> session = get_session(session_id);
	bool active = root ? root->is_visible_in_tree() : false;
	Array data;
	data.append(active);
	session.ptr()->send_message("audio_profiler_display:active", data);
	
	if (active){
		refresh_timer->start();
	}
	else{
		refresh_timer->stop();
	}
}

String get_clean_tail(const String &path_str, int count = 2) {
    PackedStringArray parts = path_str.split("/");
    int total_parts = parts.size();
    int start_index = MAX(0, total_parts - count);

    String result;
    for (int i = start_index; i < total_parts; ++i) {
        String segment = parts[i];
        result += segment;
        if (i < total_parts - 1) {
            result += "/";
        }
    }

    return result;
}

void AudioProfilerDebuggerPlugin::refresh_display() {
	if (!dirty || locked) {
		return;
	}

	dirty = false;
	displayed_players.clear();
	int playing = 0;

	for (size_t i = 0; i < latest_data.size(); i++) {
		Array element = latest_data[i];

		AudioInfo info;
		info.instance_id = element[0];
		info.instance_path = element[1];
		info.playing = element[2];
		info.type = element[3];
		if (info.playing){
			playing++;
			info.stream_path = element[4];
			info.self_volume_db = element[5];
			info.mixed_volume_db = element[6];
			info.bus = element[7];
			info.playback_position = element[8];
		}else{
			info.mixed_volume_db = -80.0;
		}

		displayed_players[info.instance_id] = info;
	}

	std::vector<const AudioInfo*> to_update;
	to_update.reserve(displayed_players.size());
	for (const KeyValue<uint64_t, AudioInfo> &x : displayed_players) {
		to_update.push_back(&x.value);
	}

	if (top10mode){
		std::sort(to_update.begin(), to_update.end(),
	    [](const AudioInfo *a, const AudioInfo *b) {
        return a->mixed_volume_db > b->mixed_volume_db;
    	});
		if(to_update.size() > 10)
			to_update.resize(10);
	}

	String filter = search->get_text().strip_edges();
	tree_item_to_id.clear();
	HashSet<uint64_t> maintained_items;

	for (const AudioInfo* info : to_update) {
		bool kept = update_tree_display_of(info, filter);
		if (kept)
			maintained_items.insert(info->instance_id);
	}

	Array leftover_items;
	// Remove old items no longer present
	for (auto &element : id_to_tree_item) {
		if (!maintained_items.has(element.key)) {
			TreeItem *to_remove = element.value;
			if (to_remove && to_remove->get_parent()) {
				to_remove->get_parent()->remove_child(to_remove);
				leftover_items.append(element.key);
			}
		}
	}

	for (size_t i = 0; i < leftover_items.size(); i++)
	{
		id_to_tree_item.erase(leftover_items[i]);
	}

	summary->set_text("Total: " + itos(displayed_players.size()) + " Playing: " + itos(playing));
}

bool AudioProfilerDebuggerPlugin::update_tree_display_of(const AudioInfo* info,  String filter)
{
		if (!filter.is_empty() &&
			!info->instance_path.contains(filter) &&
			!info->stream_path.contains(filter) &&
			!info->bus.contains(filter)) {
			return false;
		}

		if (show_only_playing_nodes && !info->playing) {
			return false;
		}

		TreeItem *it = id_to_tree_item.has(info->instance_id) ? id_to_tree_item[info->instance_id] : nullptr;
		if (!it)
			it = tree->create_item(tree->get_root());
			id_to_tree_item[info->instance_id] = it;

		tree_item_to_id[it] = info->instance_id;

		it->set_text(0, get_clean_tail(info->instance_path));
		it->set_tooltip_text(0, info->instance_path);
		it->set_text(2, info->playing ? "O" : "X");
		it->set_text(3, info->type == 3 ? "3D" : (info->type == 2 ? "2D" : ""));

		if (info->playing){
			it->set_text(1, info->stream_path.get_file());
			it->set_tooltip_text(1, info->stream_path);
			it->set_text(4, String(info->bus));
			it->set_custom_color(5, db_to_color(info->mixed_volume_db));
			it->set_text(5, String::num(info->mixed_volume_db,1));
			it->set_custom_color(6, db_to_color(info->self_volume_db));
			it->set_text(6, String::num(info->self_volume_db,1));
			it->set_text(7, String::num(info->playback_position,1));
		}

		return true;
}

void AudioProfilerDebuggerPlugin::on_only_active_toggled(bool value)
{
	show_only_playing_nodes = value;
	dirty = true;
}

void AudioProfilerDebuggerPlugin::on_locked_toggled(bool value)
{
	locked = value;
	dirty = true;
}

void AudioProfilerDebuggerPlugin::on_limit_toggled(bool value)
{
	top10mode = value;
	dirty = true;
}

void AudioProfilerDebuggerPlugin::on_tree_item_activated()
{
	TreeItem *selected = tree->get_selected();
	int column = tree->get_selected_column();
	UtilityFunctions::print(column, selected);
	uint64_t id = tree_item_to_id.get(selected);
	AudioInfo *ptr = displayed_players.getptr(id);

	if (ptr == nullptr)
		return;

	switch (column) {
		case 0: //instance path
		{
			//EditorInterface* ptr = EditorInterface::get_singleton();
			//ptr->edit_node()
			//TODO
			break;
		}
		case 1: //Stream path
		{
			EditorInterface *editor = EditorInterface::get_singleton();
			if (editor == nullptr)
				break;
			if (ptr->stream_path.is_empty())
				break;
			Ref<Resource> res = ResourceLoader::get_singleton()->load(ptr->stream_path);
			if (!res.is_valid())
				break;

			editor->edit_resource(res);
			auto dock = editor->get_file_system_dock();
			if (dock){
				dock->navigate_to_path(ptr->stream_path);
			}
			break;
		}
	}
}

Color AudioProfilerDebuggerPlugin::db_to_color(float db)
{
	float value = UtilityFunctions::db_to_linear(db);
    return Color("dedede").lerp(Color("eb4034"), MIN(1.0, value*4.0));
}

void AudioProfilerDebuggerPlugin::_notification(int p_what) {
	switch (p_what) {

		case Node::NOTIFICATION_ENTER_TREE:
			[[fallthrough]];
		case Control::NOTIFICATION_THEME_CHANGED:
			search->get_theme_icon(StringName("Search"), StringName("EditorIcons"));
			break;
	}
}
