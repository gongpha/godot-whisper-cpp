extends Control

var transcriber : WhisperMicrophoneTranscriber

func _ready() -> void :
	var whisper := WhisperFull.new()
	whisper.model = preload("res://models/ggml-tiny.en.bin")
	whisper.use_gpu = true
	whisper.language = "en"
	whisper.no_timestamps = true
	whisper.single_segment = true

	transcriber = WhisperMicrophoneTranscriber.new()
	transcriber.whisper = whisper
	transcriber.step_ms = 1000 # transcribe every 3 secs
	transcriber.length_ms = 10000 # 10 secs
	transcriber.keep_ms = 200
	add_child(transcriber)

	transcriber.transcription_text.connect(_on_transcription_text)
	transcriber.transcription_segment.connect(_on_transcription_segment)

	transcriber.start()

func _on_transcription_text(text : String) -> void :
	print("Text: ", text)

func _on_transcription_segment(segment : WhisperSegment) -> void :
	print("[%d - %d ms] %s" % [segment.t0, segment.t1, segment.text])

func _exit_tree() -> void :
	transcriber.stop()
