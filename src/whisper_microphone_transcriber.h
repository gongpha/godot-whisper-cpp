#pragma once

#ifdef _GDEXTENSION
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/audio_effect_capture.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_microphone.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/semaphore.hpp>
#include <godot_cpp/templates/safe_refcount.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
using namespace godot;
#else
#include "scene/main/node.h"
#include "servers/audio/effects/audio_effect_capture.h"
#include "scene/audio/audio_stream_player.h"
#include "servers/audio/audio_stream.h"
#include "servers/audio_server.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/os/mutex.h"
#include "core/os/semaphore.h"
#include "core/templates/safe_refcount.h"
#endif

#include "whisper_full.h"

// this class provides real-time microphone transcription using whisper
// it captures audio from the microphone, processes it in a background thread,
// and emits signals with transcribed text segments
class WhisperMicrophoneTranscriber : public Node {
	GDCLASS(WhisperMicrophoneTranscriber, Node);

	// whisper instance
	Ref<WhisperFull> whisper;

	// audio capture
	Ref<AudioEffectCapture> audio_effect;
	AudioStreamPlayer *mic_player = nullptr;
	int bus_index = -1;
	String bus_name;

	// streaming parameters (similar to whisper.cpp stream example)
	int step_ms = 3000;      // process audio every N milliseconds
	int length_ms = 10000;   // maximum audio length to process
	int keep_ms = 200;       // audio to keep from previous step (to avoid word boundary issues)

	// threading
#ifdef _GDEXTENSION
	Ref<Thread> worker_thread;
	Ref<Mutex> mtx;
	Ref<Semaphore> sem;
#else
	Thread worker_thread;
	Mutex mtx;
	Semaphore sem;
	static void _thread_callback(void *p_userdata);
#endif
	SafeFlag running;
	SafeFlag should_stop;

	// audio buffers (protected by mutex)
	PackedFloat32Array pcmf32_buffer;     // buffer for incoming audio
	PackedFloat32Array pcmf32_old;        // audio kept from previous transcription

	// results queue (protected by mutex)
	LocalVector<String> pending_texts;
	LocalVector<Ref<WhisperSegment>> pending_segments;

	// timing
	float accumulated_time = 0.0f;

	// internal methods
	void _thread_func();
	void _process_audio();
	void _setup_audio_bus();
	void _cleanup_audio_bus();
	void _emit_pending_results();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	// whisper configuration
	void set_whisper(const Ref<WhisperFull> &p_whisper);
	Ref<WhisperFull> get_whisper() const;

	// streaming parameters
	void set_step_ms(int p_step_ms);
	int get_step_ms() const;

	void set_length_ms(int p_length_ms);
	int get_length_ms() const;

	void set_keep_ms(int p_keep_ms);
	int get_keep_ms() const;

	// control methods
	bool start();
	void stop();
	bool is_running() const;

	// manual audio input (alternative to microphone)
	void push_audio_chunk(const PackedFloat32Array &p_samples);

	WhisperMicrophoneTranscriber();
	~WhisperMicrophoneTranscriber();
};
