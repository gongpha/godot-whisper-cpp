#pragma once

#ifdef _GDEXTENSION
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/templates/local_vector.hpp>
using namespace godot;
#else
#include "core/variant/typed_array.h"
#endif

#include <cfloat>

#include "whisper_model.h"

// this class represents a transcription segment result
class WhisperSegment : public RefCounted {
	GDCLASS(WhisperSegment, RefCounted);

	int64_t t0 = 0; // start time in milliseconds
	int64_t t1 = 0; // end time in milliseconds
	String text;
	bool speaker_turn_next = false;
	float no_speech_prob = 0.0f;

protected:
	static void _bind_methods();

public:
	void set_t0(int64_t p_t0) { t0 = p_t0; }
	int64_t get_t0() const { return t0; }

	void set_t1(int64_t p_t1) { t1 = p_t1; }
	int64_t get_t1() const { return t1; }

	void set_text(const String &p_text) { text = p_text; }
	String get_text() const { return text; }

	void set_speaker_turn_next(bool p_speaker_turn_next) { speaker_turn_next = p_speaker_turn_next; }
	bool get_speaker_turn_next() const { return speaker_turn_next; }

	void set_no_speech_prob(float p_no_speech_prob) { no_speech_prob = p_no_speech_prob; }
	float get_no_speech_prob() const { return no_speech_prob; }

	WhisperSegment();
	~WhisperSegment();
};

// this class represents the function "whisper_full" from whisper.cpp.
class WhisperFull : public RefCounted {
	GDCLASS(WhisperFull, RefCounted);

	Ref<WhisperModel> model;
	whisper_context *ctx = nullptr;

	// context parameters
	bool use_gpu = true;
	bool flash_attn = true;
	int gpu_device = 0;

	// full params
	whisper_sampling_strategy strategy = WHISPER_SAMPLING_GREEDY;
	int n_threads = 4;
	int n_max_text_ctx = 16384;
	int offset_ms = 0;
	int duration_ms = 0;
	bool translate = false;
	bool no_context = false;
	bool no_timestamps = false;
	bool single_segment = false;
	bool print_special = false;
	bool print_progress = false;
	bool print_realtime = false;
	bool print_timestamps = true;
	bool token_timestamps = false;
	float thold_pt = 0.01f;
	float thold_ptsum = 0.01f;
	int max_len = 0;
	bool split_on_word = false;
	int max_tokens = 0;
	bool debug_mode = false;
	int audio_ctx = 0;
	bool tdrz_enable = false;
	String suppress_regex;
	String initial_prompt;
	bool carry_initial_prompt = false;
	String language = "en";
	bool detect_language = false;
	bool suppress_blank = true;
	bool suppress_nst = false;
	float temperature = 0.0f;
	float max_initial_ts = 1.0f;
	float length_penalty = -1.0f;
	float temperature_inc = 0.2f;
	float entropy_thold = 2.4f;
	float logprob_thold = -1.0f;
	float no_speech_thold = 0.6f;

	// greedy params
	int greedy_best_of = 5;

	// beam search params
	int beam_size = 5;
	float beam_patience = -1.0f;

	// vad params
	bool vad_enable = false;
	String vad_model_path;
	float vad_threshold = 0.5f;
	int vad_min_speech_duration_ms = 250;
	int vad_min_silence_duration_ms = 100;
	float vad_max_speech_duration_s = FLT_MAX;
	int vad_speech_pad_ms = 30;
	float vad_samples_overlap = 0.1f;

	// internal - stores char data for whisper API
	CharString suppress_regex_cs;
	CharString initial_prompt_cs;
	CharString language_cs;
	CharString vad_model_path_cs;

	// internal
	bool _init_context();
	void _free_context();
	whisper_full_params _build_params();

protected:
	static void _bind_methods();

public:
    enum Strategy {
		GREEDY = WHISPER_SAMPLING_GREEDY,
		BEAM_SEARCH = WHISPER_SAMPLING_BEAM_SEARCH,
	};
    
	// model management
	void set_model(const Ref<WhisperModel> &p_model);
	Ref<WhisperModel> get_model() const;

	// context parameters
	void set_use_gpu(bool p_use_gpu);
	bool get_use_gpu() const;

	void set_flash_attn(bool p_flash_attn);
	bool get_flash_attn() const;

	void set_gpu_device(int p_gpu_device);
	int get_gpu_device() const;

	// sampling strategy
	void set_strategy(Strategy p_strategy);
	Strategy get_strategy() const;

	// full params - threads
	void set_n_threads(int p_n_threads);
	int get_n_threads() const;

	void set_n_max_text_ctx(int p_n_max_text_ctx);
	int get_n_max_text_ctx() const;

	// full params - timing
	void set_offset_ms(int p_offset_ms);
	int get_offset_ms() const;

	void set_duration_ms(int p_duration_ms);
	int get_duration_ms() const;

	// full params - flags
	void set_translate(bool p_translate);
	bool get_translate() const;

	void set_no_context(bool p_no_context);
	bool get_no_context() const;

	void set_no_timestamps(bool p_no_timestamps);
	bool get_no_timestamps() const;

	void set_single_segment(bool p_single_segment);
	bool get_single_segment() const;

	void set_print_special(bool p_print_special);
	bool get_print_special() const;

	void set_print_progress(bool p_print_progress);
	bool get_print_progress() const;

	void set_print_realtime(bool p_print_realtime);
	bool get_print_realtime() const;

	void set_print_timestamps(bool p_print_timestamps);
	bool get_print_timestamps() const;

	// full params - token timestamps
	void set_token_timestamps(bool p_token_timestamps);
	bool get_token_timestamps() const;

	void set_thold_pt(float p_thold_pt);
	float get_thold_pt() const;

	void set_thold_ptsum(float p_thold_ptsum);
	float get_thold_ptsum() const;

	void set_max_len(int p_max_len);
	int get_max_len() const;

	void set_split_on_word(bool p_split_on_word);
	bool get_split_on_word() const;

	void set_max_tokens(int p_max_tokens);
	int get_max_tokens() const;

	// full params - experimental
	void set_debug_mode(bool p_debug_mode);
	bool get_debug_mode() const;

	void set_audio_ctx(int p_audio_ctx);
	int get_audio_ctx() const;

	void set_tdrz_enable(bool p_tdrz_enable);
	bool get_tdrz_enable() const;

	void set_suppress_regex(const String &p_suppress_regex);
	String get_suppress_regex() const;

	// full params - prompt
	void set_initial_prompt(const String &p_initial_prompt);
	String get_initial_prompt() const;

	void set_carry_initial_prompt(bool p_carry_initial_prompt);
	bool get_carry_initial_prompt() const;

	// full params - language
	void set_language(const String &p_language);
	String get_language() const;

	void set_detect_language(bool p_detect_language);
	bool get_detect_language() const;

	// full params - decoding
	void set_suppress_blank(bool p_suppress_blank);
	bool get_suppress_blank() const;

	void set_suppress_nst(bool p_suppress_nst);
	bool get_suppress_nst() const;

	void set_temperature(float p_temperature);
	float get_temperature() const;

	void set_max_initial_ts(float p_max_initial_ts);
	float get_max_initial_ts() const;

	void set_length_penalty(float p_length_penalty);
	float get_length_penalty() const;

	// full params - fallback
	void set_temperature_inc(float p_temperature_inc);
	float get_temperature_inc() const;

	void set_entropy_thold(float p_entropy_thold);
	float get_entropy_thold() const;

	void set_logprob_thold(float p_logprob_thold);
	float get_logprob_thold() const;

	void set_no_speech_thold(float p_no_speech_thold);
	float get_no_speech_thold() const;

	// greedy params
	void set_greedy_best_of(int p_greedy_best_of);
	int get_greedy_best_of() const;

	// beam search params
	void set_beam_size(int p_beam_size);
	int get_beam_size() const;

	void set_beam_patience(float p_beam_patience);
	float get_beam_patience() const;

	// vad params
	void set_vad_enable(bool p_vad_enable);
	bool get_vad_enable() const;

	void set_vad_model_path(const String &p_vad_model_path);
	String get_vad_model_path() const;

	void set_vad_threshold(float p_vad_threshold);
	float get_vad_threshold() const;

	void set_vad_min_speech_duration_ms(int p_vad_min_speech_duration_ms);
	int get_vad_min_speech_duration_ms() const;

	void set_vad_min_silence_duration_ms(int p_vad_min_silence_duration_ms);
	int get_vad_min_silence_duration_ms() const;

	void set_vad_max_speech_duration_s(float p_vad_max_speech_duration_s);
	float get_vad_max_speech_duration_s() const;

	void set_vad_speech_pad_ms(int p_vad_speech_pad_ms);
	int get_vad_speech_pad_ms() const;

	void set_vad_samples_overlap(float p_vad_samples_overlap);
	float get_vad_samples_overlap() const;

	// context management
	bool is_initialized() const;
	bool init();
	void free_context();

	// model info (requires initialized context)
	bool is_multilingual() const;
	int get_model_n_vocab() const;
	int get_model_n_audio_ctx() const;
	int get_model_n_audio_state() const;
	int get_model_n_audio_head() const;
	int get_model_n_audio_layer() const;
	int get_model_n_text_ctx() const;
	int get_model_n_text_state() const;
	int get_model_n_text_head() const;
	int get_model_n_text_layer() const;
	int get_model_n_mels() const;
	int get_model_ftype() const;
	int get_model_type() const;
	String get_model_type_readable() const;

	// language utilities
	static int get_lang_max_id();
	static int get_lang_id(const String &p_lang);
	static String get_lang_str(int p_id);
	static String get_lang_str_full(int p_id);

	// audio utilities
	static PackedFloat32Array convert_stereo_to_mono_16khz(int p_from_sample_rate, const PackedVector2Array &p_stereo_data);

	// transcription methods
	// transcribe from PCM float32 samples (must be 16kHz mono)
	int transcribe(const PackedFloat32Array &p_samples);

	// transcribe from PCM float32 samples with parallel processing
	int transcribe_parallel(const PackedFloat32Array &p_samples, int p_n_processors);

	// get transcription results
	int get_segment_count() const;
	Ref<WhisperSegment> get_segment(int p_index) const;
	int get_all_segments_native(LocalVector<Ref<WhisperSegment>> &r_segments) const;
	TypedArray<WhisperSegment> get_all_segments() const;
	String get_full_text() const;

	// get detected language (after transcription with detect_language enabled)
	int get_detected_lang_id() const;
	String get_detected_language() const;

	// timing information
	Dictionary get_timings() const;
	void print_timings() const;
	void reset_timings();

	// system info
	static String get_system_info();
	static String get_version();

	// sample rate constant
	static int get_sample_rate();

	WhisperFull();
	~WhisperFull();
};

VARIANT_ENUM_CAST(WhisperFull::Strategy);