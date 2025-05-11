extends Node

func _ready() -> void:
	var tw = create_tween()
	
	tw.tween_callback(func(): add_child(Node.new())).set_delay(1.0)
	tw.tween_callback(func(): add_child(AudioStreamPlayer.new())).set_delay(1.0)
	tw.tween_callback(func(): add_child(AudioStreamPlayer2D.new())).set_delay(1.0)
	tw.tween_callback(func(): add_child(AudioStreamPlayer3D.new())).set_delay(1.0)
	
	tw.tween_callback(func():
		for x in get_children():
			x.queue_free()
		).set_delay(15.0)
