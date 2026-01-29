extends Control

func _ready() -> void :
	var model := preload("res://models/ggml-tiny.en.bin")
	var wav := preload("res://jfk.wav")
	
	var full := WhisperFull.new()
	full.use_gpu = true
	full.model = model
	full.language = "en"
	
	var data := wav.data
	var data_float : PackedFloat32Array
	match wav.format:
		AudioStreamWAV.FORMAT_8_BITS :
			@warning_ignore("integer_division")
			for i in data.size() / 2 :
				data_float.append(data.decode_s8(i * 2) * 1.0 / 128.0)
		AudioStreamWAV.FORMAT_16_BITS :
			@warning_ignore("integer_division")
			for i in data.size() / 2 :
				data_float.append(data.decode_s16(i * 2) / 32768.0)
	
	full.transcribe(data_float)
	print(full.get_full_text())
