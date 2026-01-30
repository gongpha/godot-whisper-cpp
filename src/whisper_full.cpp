#include "whisper_full.h"

#ifdef _GDEXTENSION
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
using namespace godot;
#else
#include "core/string/print_string.h"
#endif

#include <whisper.h>

/* --- WhisperSegment implementation --- */

WhisperSegment::WhisperSegment() {
}

WhisperSegment::~WhisperSegment() {
}

void WhisperSegment::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_t0", "t0"), &WhisperSegment::set_t0);
	ClassDB::bind_method(D_METHOD("get_t0"), &WhisperSegment::get_t0);

	ClassDB::bind_method(D_METHOD("set_t1", "t1"), &WhisperSegment::set_t1);
	ClassDB::bind_method(D_METHOD("get_t1"), &WhisperSegment::get_t1);

	ClassDB::bind_method(D_METHOD("set_text", "text"), &WhisperSegment::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &WhisperSegment::get_text);

	ClassDB::bind_method(D_METHOD("set_speaker_turn_next", "speaker_turn_next"), &WhisperSegment::set_speaker_turn_next);
	ClassDB::bind_method(D_METHOD("get_speaker_turn_next"), &WhisperSegment::get_speaker_turn_next);

	ClassDB::bind_method(D_METHOD("set_no_speech_prob", "no_speech_prob"), &WhisperSegment::set_no_speech_prob);
	ClassDB::bind_method(D_METHOD("get_no_speech_prob"), &WhisperSegment::get_no_speech_prob);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "t0"), "set_t0", "get_t0");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "t1"), "set_t1", "get_t1");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "speaker_turn_next"), "set_speaker_turn_next", "get_speaker_turn_next");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "no_speech_prob"), "set_no_speech_prob", "get_no_speech_prob");
}

/* --- WhisperFull implementation --- */

WhisperFull::WhisperFull() {
}

WhisperFull::~WhisperFull() {
	_free_context();
}

bool WhisperFull::_init_context() {
	if (ctx != nullptr) {
		return true; // already initialized
	}

	if (model.is_null() || model->get_bin_path().is_empty()) {
		ERR_PRINT("[WhisperFull] model is not set or has empty path");
		return false;
	}

	whisper_context_params cparams = whisper_context_default_params();
	cparams.use_gpu = use_gpu;
	cparams.flash_attn = flash_attn;
	cparams.gpu_device = gpu_device;

	CharString path_cs = model->get_bin_path().utf8();
	PackedByteArray content = FileAccess::get_file_as_bytes(model->get_bin_path());
	ctx = whisper_init_from_buffer_with_params((void *)(content.ptr()), content.size(), cparams);


	if (ctx == nullptr) {
		ERR_PRINT("[WhisperFull] failed to initialize whisper context from model: " + model->get_bin_path());
		return false;
	}

	return true;
}

void WhisperFull::_free_context() {
	if (ctx != nullptr) {
		whisper_free(ctx);
		ctx = nullptr;
	}
}

whisper_full_params WhisperFull::_build_params() {
	whisper_full_params wparams = whisper_full_default_params(strategy);

	wparams.n_threads = n_threads;
	wparams.n_max_text_ctx = n_max_text_ctx;
	wparams.offset_ms = offset_ms;
	wparams.duration_ms = duration_ms;

	wparams.translate = translate;
	wparams.no_context = no_context;
	wparams.no_timestamps = no_timestamps;
	wparams.single_segment = single_segment;
	wparams.print_special = print_special;
	wparams.print_progress = print_progress;
	wparams.print_realtime = print_realtime;
	wparams.print_timestamps = print_timestamps;

	wparams.token_timestamps = token_timestamps;
	wparams.thold_pt = thold_pt;
	wparams.thold_ptsum = thold_ptsum;
	wparams.max_len = max_len;
	wparams.split_on_word = split_on_word;
	wparams.max_tokens = max_tokens;

	wparams.debug_mode = debug_mode;
	wparams.audio_ctx = audio_ctx;
	wparams.tdrz_enable = tdrz_enable;

	// store CharString to keep memory valid
	suppress_regex_cs = suppress_regex.utf8();
	wparams.suppress_regex = suppress_regex.is_empty() ? nullptr : suppress_regex_cs.get_data();

	initial_prompt_cs = initial_prompt.utf8();
	wparams.initial_prompt = initial_prompt.is_empty() ? nullptr : initial_prompt_cs.get_data();
	wparams.carry_initial_prompt = carry_initial_prompt;
	wparams.prompt_tokens = nullptr;
	wparams.prompt_n_tokens = 0;

	language_cs = language.utf8();
	wparams.language = language.is_empty() || language == "auto" ? nullptr : language_cs.get_data();
	wparams.detect_language = detect_language;

	wparams.suppress_blank = suppress_blank;
	wparams.suppress_nst = suppress_nst;

	wparams.temperature = temperature;
	wparams.max_initial_ts = max_initial_ts;
	wparams.length_penalty = length_penalty;

	wparams.temperature_inc = temperature_inc;
	wparams.entropy_thold = entropy_thold;
	wparams.logprob_thold = logprob_thold;
	wparams.no_speech_thold = no_speech_thold;

	wparams.greedy.best_of = greedy_best_of;

	wparams.beam_search.beam_size = beam_size;
	wparams.beam_search.patience = beam_patience;

	// vad params
	wparams.vad = vad_enable;
	vad_model_path_cs = vad_model_path.utf8();
	wparams.vad_model_path = vad_model_path.is_empty() ? nullptr : vad_model_path_cs.get_data();
	wparams.vad_params.threshold = vad_threshold;
	wparams.vad_params.min_speech_duration_ms = vad_min_speech_duration_ms;
	wparams.vad_params.min_silence_duration_ms = vad_min_silence_duration_ms;
	wparams.vad_params.max_speech_duration_s = vad_max_speech_duration_s;
	wparams.vad_params.speech_pad_ms = vad_speech_pad_ms;
	wparams.vad_params.samples_overlap = vad_samples_overlap;

	// callbacks (not exposed to GDScript for now)
	wparams.new_segment_callback = nullptr;
	wparams.new_segment_callback_user_data = nullptr;
	wparams.progress_callback = nullptr;
	wparams.progress_callback_user_data = nullptr;
	wparams.encoder_begin_callback = nullptr;
	wparams.encoder_begin_callback_user_data = nullptr;
	wparams.abort_callback = nullptr;
	wparams.abort_callback_user_data = nullptr;
	wparams.logits_filter_callback = nullptr;
	wparams.logits_filter_callback_user_data = nullptr;

	// grammar (not exposed for now)
	wparams.grammar_rules = nullptr;
	wparams.n_grammar_rules = 0;
	wparams.i_start_rule = 0;
	wparams.grammar_penalty = 100.0f;

	return wparams;
}

void WhisperFull::_bind_methods() {
	// model management
	ClassDB::bind_method(D_METHOD("set_model", "model"), &WhisperFull::set_model);
	ClassDB::bind_method(D_METHOD("get_model"), &WhisperFull::get_model);

	// context parameters
	ClassDB::bind_method(D_METHOD("set_use_gpu", "use_gpu"), &WhisperFull::set_use_gpu);
	ClassDB::bind_method(D_METHOD("get_use_gpu"), &WhisperFull::get_use_gpu);

	ClassDB::bind_method(D_METHOD("set_flash_attn", "flash_attn"), &WhisperFull::set_flash_attn);
	ClassDB::bind_method(D_METHOD("get_flash_attn"), &WhisperFull::get_flash_attn);

	ClassDB::bind_method(D_METHOD("set_gpu_device", "gpu_device"), &WhisperFull::set_gpu_device);
	ClassDB::bind_method(D_METHOD("get_gpu_device"), &WhisperFull::get_gpu_device);

	// sampling strategy
	ClassDB::bind_method(D_METHOD("set_strategy", "strategy"), &WhisperFull::set_strategy);
	ClassDB::bind_method(D_METHOD("get_strategy"), &WhisperFull::get_strategy);

	// full params - threads
	ClassDB::bind_method(D_METHOD("set_n_threads", "n_threads"), &WhisperFull::set_n_threads);
	ClassDB::bind_method(D_METHOD("get_n_threads"), &WhisperFull::get_n_threads);

	ClassDB::bind_method(D_METHOD("set_n_max_text_ctx", "n_max_text_ctx"), &WhisperFull::set_n_max_text_ctx);
	ClassDB::bind_method(D_METHOD("get_n_max_text_ctx"), &WhisperFull::get_n_max_text_ctx);

	// full params - timing
	ClassDB::bind_method(D_METHOD("set_offset_ms", "offset_ms"), &WhisperFull::set_offset_ms);
	ClassDB::bind_method(D_METHOD("get_offset_ms"), &WhisperFull::get_offset_ms);

	ClassDB::bind_method(D_METHOD("set_duration_ms", "duration_ms"), &WhisperFull::set_duration_ms);
	ClassDB::bind_method(D_METHOD("get_duration_ms"), &WhisperFull::get_duration_ms);

	// full params - flags
	ClassDB::bind_method(D_METHOD("set_translate", "translate"), &WhisperFull::set_translate);
	ClassDB::bind_method(D_METHOD("get_translate"), &WhisperFull::get_translate);

	ClassDB::bind_method(D_METHOD("set_no_context", "no_context"), &WhisperFull::set_no_context);
	ClassDB::bind_method(D_METHOD("get_no_context"), &WhisperFull::get_no_context);

	ClassDB::bind_method(D_METHOD("set_no_timestamps", "no_timestamps"), &WhisperFull::set_no_timestamps);
	ClassDB::bind_method(D_METHOD("get_no_timestamps"), &WhisperFull::get_no_timestamps);

	ClassDB::bind_method(D_METHOD("set_single_segment", "single_segment"), &WhisperFull::set_single_segment);
	ClassDB::bind_method(D_METHOD("get_single_segment"), &WhisperFull::get_single_segment);

	ClassDB::bind_method(D_METHOD("set_print_special", "print_special"), &WhisperFull::set_print_special);
	ClassDB::bind_method(D_METHOD("get_print_special"), &WhisperFull::get_print_special);

	ClassDB::bind_method(D_METHOD("set_print_progress", "print_progress"), &WhisperFull::set_print_progress);
	ClassDB::bind_method(D_METHOD("get_print_progress"), &WhisperFull::get_print_progress);

	ClassDB::bind_method(D_METHOD("set_print_realtime", "print_realtime"), &WhisperFull::set_print_realtime);
	ClassDB::bind_method(D_METHOD("get_print_realtime"), &WhisperFull::get_print_realtime);

	ClassDB::bind_method(D_METHOD("set_print_timestamps", "print_timestamps"), &WhisperFull::set_print_timestamps);
	ClassDB::bind_method(D_METHOD("get_print_timestamps"), &WhisperFull::get_print_timestamps);

	// full params - token timestamps
	ClassDB::bind_method(D_METHOD("set_token_timestamps", "token_timestamps"), &WhisperFull::set_token_timestamps);
	ClassDB::bind_method(D_METHOD("get_token_timestamps"), &WhisperFull::get_token_timestamps);

	ClassDB::bind_method(D_METHOD("set_thold_pt", "thold_pt"), &WhisperFull::set_thold_pt);
	ClassDB::bind_method(D_METHOD("get_thold_pt"), &WhisperFull::get_thold_pt);

	ClassDB::bind_method(D_METHOD("set_thold_ptsum", "thold_ptsum"), &WhisperFull::set_thold_ptsum);
	ClassDB::bind_method(D_METHOD("get_thold_ptsum"), &WhisperFull::get_thold_ptsum);

	ClassDB::bind_method(D_METHOD("set_max_len", "max_len"), &WhisperFull::set_max_len);
	ClassDB::bind_method(D_METHOD("get_max_len"), &WhisperFull::get_max_len);

	ClassDB::bind_method(D_METHOD("set_split_on_word", "split_on_word"), &WhisperFull::set_split_on_word);
	ClassDB::bind_method(D_METHOD("get_split_on_word"), &WhisperFull::get_split_on_word);

	ClassDB::bind_method(D_METHOD("set_max_tokens", "max_tokens"), &WhisperFull::set_max_tokens);
	ClassDB::bind_method(D_METHOD("get_max_tokens"), &WhisperFull::get_max_tokens);

	// full params - experimental
	ClassDB::bind_method(D_METHOD("set_debug_mode", "debug_mode"), &WhisperFull::set_debug_mode);
	ClassDB::bind_method(D_METHOD("get_debug_mode"), &WhisperFull::get_debug_mode);

	ClassDB::bind_method(D_METHOD("set_audio_ctx", "audio_ctx"), &WhisperFull::set_audio_ctx);
	ClassDB::bind_method(D_METHOD("get_audio_ctx"), &WhisperFull::get_audio_ctx);

	ClassDB::bind_method(D_METHOD("set_tdrz_enable", "tdrz_enable"), &WhisperFull::set_tdrz_enable);
	ClassDB::bind_method(D_METHOD("get_tdrz_enable"), &WhisperFull::get_tdrz_enable);

	ClassDB::bind_method(D_METHOD("set_suppress_regex", "suppress_regex"), &WhisperFull::set_suppress_regex);
	ClassDB::bind_method(D_METHOD("get_suppress_regex"), &WhisperFull::get_suppress_regex);

	// full params - prompt
	ClassDB::bind_method(D_METHOD("set_initial_prompt", "initial_prompt"), &WhisperFull::set_initial_prompt);
	ClassDB::bind_method(D_METHOD("get_initial_prompt"), &WhisperFull::get_initial_prompt);

	ClassDB::bind_method(D_METHOD("set_carry_initial_prompt", "carry_initial_prompt"), &WhisperFull::set_carry_initial_prompt);
	ClassDB::bind_method(D_METHOD("get_carry_initial_prompt"), &WhisperFull::get_carry_initial_prompt);

	// full params - language
	ClassDB::bind_method(D_METHOD("set_language", "language"), &WhisperFull::set_language);
	ClassDB::bind_method(D_METHOD("get_language"), &WhisperFull::get_language);

	ClassDB::bind_method(D_METHOD("set_detect_language", "detect_language"), &WhisperFull::set_detect_language);
	ClassDB::bind_method(D_METHOD("get_detect_language"), &WhisperFull::get_detect_language);

	// full params - decoding
	ClassDB::bind_method(D_METHOD("set_suppress_blank", "suppress_blank"), &WhisperFull::set_suppress_blank);
	ClassDB::bind_method(D_METHOD("get_suppress_blank"), &WhisperFull::get_suppress_blank);

	ClassDB::bind_method(D_METHOD("set_suppress_nst", "suppress_nst"), &WhisperFull::set_suppress_nst);
	ClassDB::bind_method(D_METHOD("get_suppress_nst"), &WhisperFull::get_suppress_nst);

	ClassDB::bind_method(D_METHOD("set_temperature", "temperature"), &WhisperFull::set_temperature);
	ClassDB::bind_method(D_METHOD("get_temperature"), &WhisperFull::get_temperature);

	ClassDB::bind_method(D_METHOD("set_max_initial_ts", "max_initial_ts"), &WhisperFull::set_max_initial_ts);
	ClassDB::bind_method(D_METHOD("get_max_initial_ts"), &WhisperFull::get_max_initial_ts);

	ClassDB::bind_method(D_METHOD("set_length_penalty", "length_penalty"), &WhisperFull::set_length_penalty);
	ClassDB::bind_method(D_METHOD("get_length_penalty"), &WhisperFull::get_length_penalty);

	// full params - fallback
	ClassDB::bind_method(D_METHOD("set_temperature_inc", "temperature_inc"), &WhisperFull::set_temperature_inc);
	ClassDB::bind_method(D_METHOD("get_temperature_inc"), &WhisperFull::get_temperature_inc);

	ClassDB::bind_method(D_METHOD("set_entropy_thold", "entropy_thold"), &WhisperFull::set_entropy_thold);
	ClassDB::bind_method(D_METHOD("get_entropy_thold"), &WhisperFull::get_entropy_thold);

	ClassDB::bind_method(D_METHOD("set_logprob_thold", "logprob_thold"), &WhisperFull::set_logprob_thold);
	ClassDB::bind_method(D_METHOD("get_logprob_thold"), &WhisperFull::get_logprob_thold);

	ClassDB::bind_method(D_METHOD("set_no_speech_thold", "no_speech_thold"), &WhisperFull::set_no_speech_thold);
	ClassDB::bind_method(D_METHOD("get_no_speech_thold"), &WhisperFull::get_no_speech_thold);

	// greedy params
	ClassDB::bind_method(D_METHOD("set_greedy_best_of", "greedy_best_of"), &WhisperFull::set_greedy_best_of);
	ClassDB::bind_method(D_METHOD("get_greedy_best_of"), &WhisperFull::get_greedy_best_of);

	// beam search params
	ClassDB::bind_method(D_METHOD("set_beam_size", "beam_size"), &WhisperFull::set_beam_size);
	ClassDB::bind_method(D_METHOD("get_beam_size"), &WhisperFull::get_beam_size);

	ClassDB::bind_method(D_METHOD("set_beam_patience", "beam_patience"), &WhisperFull::set_beam_patience);
	ClassDB::bind_method(D_METHOD("get_beam_patience"), &WhisperFull::get_beam_patience);

	// vad params
	ClassDB::bind_method(D_METHOD("set_vad_enable", "vad_enable"), &WhisperFull::set_vad_enable);
	ClassDB::bind_method(D_METHOD("get_vad_enable"), &WhisperFull::get_vad_enable);

	ClassDB::bind_method(D_METHOD("set_vad_model_path", "vad_model_path"), &WhisperFull::set_vad_model_path);
	ClassDB::bind_method(D_METHOD("get_vad_model_path"), &WhisperFull::get_vad_model_path);

	ClassDB::bind_method(D_METHOD("set_vad_threshold", "vad_threshold"), &WhisperFull::set_vad_threshold);
	ClassDB::bind_method(D_METHOD("get_vad_threshold"), &WhisperFull::get_vad_threshold);

	ClassDB::bind_method(D_METHOD("set_vad_min_speech_duration_ms", "vad_min_speech_duration_ms"), &WhisperFull::set_vad_min_speech_duration_ms);
	ClassDB::bind_method(D_METHOD("get_vad_min_speech_duration_ms"), &WhisperFull::get_vad_min_speech_duration_ms);

	ClassDB::bind_method(D_METHOD("set_vad_min_silence_duration_ms", "vad_min_silence_duration_ms"), &WhisperFull::set_vad_min_silence_duration_ms);
	ClassDB::bind_method(D_METHOD("get_vad_min_silence_duration_ms"), &WhisperFull::get_vad_min_silence_duration_ms);

	ClassDB::bind_method(D_METHOD("set_vad_max_speech_duration_s", "vad_max_speech_duration_s"), &WhisperFull::set_vad_max_speech_duration_s);
	ClassDB::bind_method(D_METHOD("get_vad_max_speech_duration_s"), &WhisperFull::get_vad_max_speech_duration_s);

	ClassDB::bind_method(D_METHOD("set_vad_speech_pad_ms", "vad_speech_pad_ms"), &WhisperFull::set_vad_speech_pad_ms);
	ClassDB::bind_method(D_METHOD("get_vad_speech_pad_ms"), &WhisperFull::get_vad_speech_pad_ms);

	ClassDB::bind_method(D_METHOD("set_vad_samples_overlap", "vad_samples_overlap"), &WhisperFull::set_vad_samples_overlap);
	ClassDB::bind_method(D_METHOD("get_vad_samples_overlap"), &WhisperFull::get_vad_samples_overlap);

	// context management
	ClassDB::bind_method(D_METHOD("is_initialized"), &WhisperFull::is_initialized);
	ClassDB::bind_method(D_METHOD("init"), &WhisperFull::init);
	ClassDB::bind_method(D_METHOD("free_context"), &WhisperFull::free_context);

	// model info
	ClassDB::bind_method(D_METHOD("is_multilingual"), &WhisperFull::is_multilingual);
	ClassDB::bind_method(D_METHOD("get_model_n_vocab"), &WhisperFull::get_model_n_vocab);
	ClassDB::bind_method(D_METHOD("get_model_n_audio_ctx"), &WhisperFull::get_model_n_audio_ctx);
	ClassDB::bind_method(D_METHOD("get_model_n_audio_state"), &WhisperFull::get_model_n_audio_state);
	ClassDB::bind_method(D_METHOD("get_model_n_audio_head"), &WhisperFull::get_model_n_audio_head);
	ClassDB::bind_method(D_METHOD("get_model_n_audio_layer"), &WhisperFull::get_model_n_audio_layer);
	ClassDB::bind_method(D_METHOD("get_model_n_text_ctx"), &WhisperFull::get_model_n_text_ctx);
	ClassDB::bind_method(D_METHOD("get_model_n_text_state"), &WhisperFull::get_model_n_text_state);
	ClassDB::bind_method(D_METHOD("get_model_n_text_head"), &WhisperFull::get_model_n_text_head);
	ClassDB::bind_method(D_METHOD("get_model_n_text_layer"), &WhisperFull::get_model_n_text_layer);
	ClassDB::bind_method(D_METHOD("get_model_n_mels"), &WhisperFull::get_model_n_mels);
	ClassDB::bind_method(D_METHOD("get_model_ftype"), &WhisperFull::get_model_ftype);
	ClassDB::bind_method(D_METHOD("get_model_type"), &WhisperFull::get_model_type);
	ClassDB::bind_method(D_METHOD("get_model_type_readable"), &WhisperFull::get_model_type_readable);

	// language utilities
	ClassDB::bind_static_method("WhisperFull", D_METHOD("get_lang_max_id"), &WhisperFull::get_lang_max_id);
	ClassDB::bind_static_method("WhisperFull", D_METHOD("get_lang_id", "lang"), &WhisperFull::get_lang_id);
	ClassDB::bind_static_method("WhisperFull", D_METHOD("get_lang_str", "id"), &WhisperFull::get_lang_str);
	ClassDB::bind_static_method("WhisperFull", D_METHOD("get_lang_str_full", "id"), &WhisperFull::get_lang_str_full);


	// methods


	ClassDB::bind_method(D_METHOD("transcribe", "samples"), &WhisperFull::transcribe);
	ClassDB::bind_method(D_METHOD("transcribe_parallel", "samples", "n_processors"), &WhisperFull::transcribe_parallel);

	ClassDB::bind_method(D_METHOD("get_segment_count"), &WhisperFull::get_segment_count);
	ClassDB::bind_method(D_METHOD("get_segment", "index"), &WhisperFull::get_segment);
	ClassDB::bind_method(D_METHOD("get_all_segments"), &WhisperFull::get_all_segments);
	ClassDB::bind_method(D_METHOD("get_full_text"), &WhisperFull::get_full_text);

	ClassDB::bind_method(D_METHOD("get_detected_lang_id"), &WhisperFull::get_detected_lang_id);
	ClassDB::bind_method(D_METHOD("get_detected_language"), &WhisperFull::get_detected_language);

	ClassDB::bind_method(D_METHOD("get_timings"), &WhisperFull::get_timings);
	ClassDB::bind_method(D_METHOD("print_timings"), &WhisperFull::print_timings);
	ClassDB::bind_method(D_METHOD("reset_timings"), &WhisperFull::reset_timings);

	ClassDB::bind_static_method("WhisperFull", D_METHOD("get_system_info"), &WhisperFull::get_system_info);
	ClassDB::bind_static_method("WhisperFull", D_METHOD("get_version"), &WhisperFull::get_version);
	ClassDB::bind_static_method("WhisperFull", D_METHOD("get_sample_rate"), &WhisperFull::get_sample_rate);

	// utilities
	ClassDB::bind_static_method("WhisperFull", D_METHOD("convert_stereo_to_mono_16khz", "from_sample_rate", "samples"), &WhisperFull::convert_stereo_to_mono_16khz);

	// note : this class is not Resource-based. so there's no way to display properties in the inspector ?

	ADD_GROUP("Model", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "model", PROPERTY_HINT_RESOURCE_TYPE, "WhisperModel"), "set_model", "get_model");

	ADD_GROUP("Context", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_gpu"), "set_use_gpu", "get_use_gpu");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flash_attn"), "set_flash_attn", "get_flash_attn");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gpu_device"), "set_gpu_device", "get_gpu_device");

	ADD_GROUP("Strategy", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "strategy", PROPERTY_HINT_ENUM, "Greedy,BeamSearch"), "set_strategy", "get_strategy");

	ADD_GROUP("Threading", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "n_threads"), "set_n_threads", "get_n_threads");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "n_max_text_ctx"), "set_n_max_text_ctx", "get_n_max_text_ctx");

	ADD_GROUP("Timing", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "offset_ms"), "set_offset_ms", "get_offset_ms");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "duration_ms"), "set_duration_ms", "get_duration_ms");

	ADD_GROUP("Options", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "translate"), "set_translate", "get_translate");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "no_context"), "set_no_context", "get_no_context");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "no_timestamps"), "set_no_timestamps", "get_no_timestamps");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "single_segment"), "set_single_segment", "get_single_segment");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "print_special"), "set_print_special", "get_print_special");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "print_progress"), "set_print_progress", "get_print_progress");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "print_realtime"), "set_print_realtime", "get_print_realtime");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "print_timestamps"), "set_print_timestamps", "get_print_timestamps");

	ADD_GROUP("Token Timestamps", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "token_timestamps"), "set_token_timestamps", "get_token_timestamps");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "thold_pt"), "set_thold_pt", "get_thold_pt");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "thold_ptsum"), "set_thold_ptsum", "get_thold_ptsum");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_len"), "set_max_len", "get_max_len");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "split_on_word"), "set_split_on_word", "get_split_on_word");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_tokens"), "set_max_tokens", "get_max_tokens");

	ADD_GROUP("Experimental", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_mode"), "set_debug_mode", "get_debug_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "audio_ctx"), "set_audio_ctx", "get_audio_ctx");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tdrz_enable"), "set_tdrz_enable", "get_tdrz_enable");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "suppress_regex"), "set_suppress_regex", "get_suppress_regex");

	ADD_GROUP("Prompt", "");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "initial_prompt"), "set_initial_prompt", "get_initial_prompt");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "carry_initial_prompt"), "set_carry_initial_prompt", "get_carry_initial_prompt");

	ADD_GROUP("Language", "");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "language"), "set_language", "get_language");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "detect_language"), "set_detect_language", "get_detect_language");

	ADD_GROUP("Decoding", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "suppress_blank"), "set_suppress_blank", "get_suppress_blank");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "suppress_nst"), "set_suppress_nst", "get_suppress_nst");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "temperature"), "set_temperature", "get_temperature");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_initial_ts"), "set_max_initial_ts", "get_max_initial_ts");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "length_penalty"), "set_length_penalty", "get_length_penalty");

	ADD_GROUP("Fallback", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "temperature_inc"), "set_temperature_inc", "get_temperature_inc");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "entropy_thold"), "set_entropy_thold", "get_entropy_thold");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "logprob_thold"), "set_logprob_thold", "get_logprob_thold");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "no_speech_thold"), "set_no_speech_thold", "get_no_speech_thold");

	ADD_GROUP("Greedy", "greedy_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "greedy_best_of"), "set_greedy_best_of", "get_greedy_best_of");

	ADD_GROUP("Beam Search", "beam_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "beam_size"), "set_beam_size", "get_beam_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "beam_patience"), "set_beam_patience", "get_beam_patience");

	ADD_GROUP("VAD", "vad_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "vad_enable"), "set_vad_enable", "get_vad_enable");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "vad_model_path", PROPERTY_HINT_FILE, "*.bin"), "set_vad_model_path", "get_vad_model_path");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vad_threshold"), "set_vad_threshold", "get_vad_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "vad_min_speech_duration_ms"), "set_vad_min_speech_duration_ms", "get_vad_min_speech_duration_ms");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "vad_min_silence_duration_ms"), "set_vad_min_silence_duration_ms", "get_vad_min_silence_duration_ms");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vad_max_speech_duration_s"), "set_vad_max_speech_duration_s", "get_vad_max_speech_duration_s");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "vad_speech_pad_ms"), "set_vad_speech_pad_ms", "get_vad_speech_pad_ms");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vad_samples_overlap"), "set_vad_samples_overlap", "get_vad_samples_overlap");

	// enum binding
	BIND_ENUM_CONSTANT(GREEDY);
	BIND_ENUM_CONSTANT(BEAM_SEARCH);
}

/* --- getters and setters --- */

void WhisperFull::set_model(const Ref<WhisperModel> &p_model) {
	// if model changes, free existing context
	if (model != p_model) {
		_free_context();
	}
	model = p_model;
}

Ref<WhisperModel> WhisperFull::get_model() const {
	return model;
}

void WhisperFull::set_use_gpu(bool p_use_gpu) {
	if (use_gpu != p_use_gpu) {
		_free_context(); // need to reinitialize
	}
	use_gpu = p_use_gpu;
}

bool WhisperFull::get_use_gpu() const {
	return use_gpu;
}

void WhisperFull::set_flash_attn(bool p_flash_attn) {
	if (flash_attn != p_flash_attn) {
		_free_context();
	}
	flash_attn = p_flash_attn;
}

bool WhisperFull::get_flash_attn() const {
	return flash_attn;
}

void WhisperFull::set_gpu_device(int p_gpu_device) {
	if (gpu_device != p_gpu_device) {
		_free_context();
	}
	gpu_device = p_gpu_device;
}

int WhisperFull::get_gpu_device() const {
	return gpu_device;
}

void WhisperFull::set_strategy(Strategy p_strategy) {
	strategy = (whisper_sampling_strategy)p_strategy;
}

WhisperFull::Strategy WhisperFull::get_strategy() const {
	return (Strategy)strategy;
}

void WhisperFull::set_n_threads(int p_n_threads) {
	n_threads = p_n_threads;
}

int WhisperFull::get_n_threads() const {
	return n_threads;
}

void WhisperFull::set_n_max_text_ctx(int p_n_max_text_ctx) {
	n_max_text_ctx = p_n_max_text_ctx;
}

int WhisperFull::get_n_max_text_ctx() const {
	return n_max_text_ctx;
}

void WhisperFull::set_offset_ms(int p_offset_ms) {
	offset_ms = p_offset_ms;
}

int WhisperFull::get_offset_ms() const {
	return offset_ms;
}

void WhisperFull::set_duration_ms(int p_duration_ms) {
	duration_ms = p_duration_ms;
}

int WhisperFull::get_duration_ms() const {
	return duration_ms;
}

void WhisperFull::set_translate(bool p_translate) {
	translate = p_translate;
}

bool WhisperFull::get_translate() const {
	return translate;
}

void WhisperFull::set_no_context(bool p_no_context) {
	no_context = p_no_context;
}

bool WhisperFull::get_no_context() const {
	return no_context;
}

void WhisperFull::set_no_timestamps(bool p_no_timestamps) {
	no_timestamps = p_no_timestamps;
}

bool WhisperFull::get_no_timestamps() const {
	return no_timestamps;
}

void WhisperFull::set_single_segment(bool p_single_segment) {
	single_segment = p_single_segment;
}

bool WhisperFull::get_single_segment() const {
	return single_segment;
}

void WhisperFull::set_print_special(bool p_print_special) {
	print_special = p_print_special;
}

bool WhisperFull::get_print_special() const {
	return print_special;
}

void WhisperFull::set_print_progress(bool p_print_progress) {
	print_progress = p_print_progress;
}

bool WhisperFull::get_print_progress() const {
	return print_progress;
}

void WhisperFull::set_print_realtime(bool p_print_realtime) {
	print_realtime = p_print_realtime;
}

bool WhisperFull::get_print_realtime() const {
	return print_realtime;
}

void WhisperFull::set_print_timestamps(bool p_print_timestamps) {
	print_timestamps = p_print_timestamps;
}

bool WhisperFull::get_print_timestamps() const {
	return print_timestamps;
}

void WhisperFull::set_token_timestamps(bool p_token_timestamps) {
	token_timestamps = p_token_timestamps;
}

bool WhisperFull::get_token_timestamps() const {
	return token_timestamps;
}

void WhisperFull::set_thold_pt(float p_thold_pt) {
	thold_pt = p_thold_pt;
}

float WhisperFull::get_thold_pt() const {
	return thold_pt;
}

void WhisperFull::set_thold_ptsum(float p_thold_ptsum) {
	thold_ptsum = p_thold_ptsum;
}

float WhisperFull::get_thold_ptsum() const {
	return thold_ptsum;
}

void WhisperFull::set_max_len(int p_max_len) {
	max_len = p_max_len;
}

int WhisperFull::get_max_len() const {
	return max_len;
}

void WhisperFull::set_split_on_word(bool p_split_on_word) {
	split_on_word = p_split_on_word;
}

bool WhisperFull::get_split_on_word() const {
	return split_on_word;
}

void WhisperFull::set_max_tokens(int p_max_tokens) {
	max_tokens = p_max_tokens;
}

int WhisperFull::get_max_tokens() const {
	return max_tokens;
}

void WhisperFull::set_debug_mode(bool p_debug_mode) {
	debug_mode = p_debug_mode;
}

bool WhisperFull::get_debug_mode() const {
	return debug_mode;
}

void WhisperFull::set_audio_ctx(int p_audio_ctx) {
	audio_ctx = p_audio_ctx;
}

int WhisperFull::get_audio_ctx() const {
	return audio_ctx;
}

void WhisperFull::set_tdrz_enable(bool p_tdrz_enable) {
	tdrz_enable = p_tdrz_enable;
}

bool WhisperFull::get_tdrz_enable() const {
	return tdrz_enable;
}

void WhisperFull::set_suppress_regex(const String &p_suppress_regex) {
	suppress_regex = p_suppress_regex;
}

String WhisperFull::get_suppress_regex() const {
	return suppress_regex;
}

void WhisperFull::set_initial_prompt(const String &p_initial_prompt) {
	initial_prompt = p_initial_prompt;
}

String WhisperFull::get_initial_prompt() const {
	return initial_prompt;
}

void WhisperFull::set_carry_initial_prompt(bool p_carry_initial_prompt) {
	carry_initial_prompt = p_carry_initial_prompt;
}

bool WhisperFull::get_carry_initial_prompt() const {
	return carry_initial_prompt;
}

void WhisperFull::set_language(const String &p_language) {
	language = p_language;
}

String WhisperFull::get_language() const {
	return language;
}

void WhisperFull::set_detect_language(bool p_detect_language) {
	detect_language = p_detect_language;
}

bool WhisperFull::get_detect_language() const {
	return detect_language;
}

void WhisperFull::set_suppress_blank(bool p_suppress_blank) {
	suppress_blank = p_suppress_blank;
}

bool WhisperFull::get_suppress_blank() const {
	return suppress_blank;
}

void WhisperFull::set_suppress_nst(bool p_suppress_nst) {
	suppress_nst = p_suppress_nst;
}

bool WhisperFull::get_suppress_nst() const {
	return suppress_nst;
}

void WhisperFull::set_temperature(float p_temperature) {
	temperature = p_temperature;
}

float WhisperFull::get_temperature() const {
	return temperature;
}

void WhisperFull::set_max_initial_ts(float p_max_initial_ts) {
	max_initial_ts = p_max_initial_ts;
}

float WhisperFull::get_max_initial_ts() const {
	return max_initial_ts;
}

void WhisperFull::set_length_penalty(float p_length_penalty) {
	length_penalty = p_length_penalty;
}

float WhisperFull::get_length_penalty() const {
	return length_penalty;
}

void WhisperFull::set_temperature_inc(float p_temperature_inc) {
	temperature_inc = p_temperature_inc;
}

float WhisperFull::get_temperature_inc() const {
	return temperature_inc;
}

void WhisperFull::set_entropy_thold(float p_entropy_thold) {
	entropy_thold = p_entropy_thold;
}

float WhisperFull::get_entropy_thold() const {
	return entropy_thold;
}

void WhisperFull::set_logprob_thold(float p_logprob_thold) {
	logprob_thold = p_logprob_thold;
}

float WhisperFull::get_logprob_thold() const {
	return logprob_thold;
}

void WhisperFull::set_no_speech_thold(float p_no_speech_thold) {
	no_speech_thold = p_no_speech_thold;
}

float WhisperFull::get_no_speech_thold() const {
	return no_speech_thold;
}

void WhisperFull::set_greedy_best_of(int p_greedy_best_of) {
	greedy_best_of = p_greedy_best_of;
}

int WhisperFull::get_greedy_best_of() const {
	return greedy_best_of;
}

void WhisperFull::set_beam_size(int p_beam_size) {
	beam_size = p_beam_size;
}

int WhisperFull::get_beam_size() const {
	return beam_size;
}

void WhisperFull::set_beam_patience(float p_beam_patience) {
	beam_patience = p_beam_patience;
}

float WhisperFull::get_beam_patience() const {
	return beam_patience;
}

void WhisperFull::set_vad_enable(bool p_vad_enable) {
	vad_enable = p_vad_enable;
}

bool WhisperFull::get_vad_enable() const {
	return vad_enable;
}

void WhisperFull::set_vad_model_path(const String &p_vad_model_path) {
	vad_model_path = p_vad_model_path;
}

String WhisperFull::get_vad_model_path() const {
	return vad_model_path;
}

void WhisperFull::set_vad_threshold(float p_vad_threshold) {
	vad_threshold = p_vad_threshold;
}

float WhisperFull::get_vad_threshold() const {
	return vad_threshold;
}

void WhisperFull::set_vad_min_speech_duration_ms(int p_vad_min_speech_duration_ms) {
	vad_min_speech_duration_ms = p_vad_min_speech_duration_ms;
}

int WhisperFull::get_vad_min_speech_duration_ms() const {
	return vad_min_speech_duration_ms;
}

void WhisperFull::set_vad_min_silence_duration_ms(int p_vad_min_silence_duration_ms) {
	vad_min_silence_duration_ms = p_vad_min_silence_duration_ms;
}

int WhisperFull::get_vad_min_silence_duration_ms() const {
	return vad_min_silence_duration_ms;
}

void WhisperFull::set_vad_max_speech_duration_s(float p_vad_max_speech_duration_s) {
	vad_max_speech_duration_s = p_vad_max_speech_duration_s;
}

float WhisperFull::get_vad_max_speech_duration_s() const {
	return vad_max_speech_duration_s;
}

void WhisperFull::set_vad_speech_pad_ms(int p_vad_speech_pad_ms) {
	vad_speech_pad_ms = p_vad_speech_pad_ms;
}

int WhisperFull::get_vad_speech_pad_ms() const {
	return vad_speech_pad_ms;
}

void WhisperFull::set_vad_samples_overlap(float p_vad_samples_overlap) {
	vad_samples_overlap = p_vad_samples_overlap;
}

float WhisperFull::get_vad_samples_overlap() const {
	return vad_samples_overlap;
}

/* --- context management --- */

bool WhisperFull::is_initialized() const {
	return ctx != nullptr;
}

bool WhisperFull::init() {
	return _init_context();
}

void WhisperFull::free_context() {
	_free_context();
}

/* --- model info --- */

bool WhisperFull::is_multilingual() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, false, "[WhisperFull] context not initialized");
	return whisper_is_multilingual(ctx) != 0;
}

int WhisperFull::get_model_n_vocab() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_n_vocab(ctx);
}

int WhisperFull::get_model_n_audio_ctx() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_n_audio_ctx(ctx);
}

int WhisperFull::get_model_n_audio_state() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_n_audio_state(ctx);
}

int WhisperFull::get_model_n_audio_head() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_n_audio_head(ctx);
}

int WhisperFull::get_model_n_audio_layer() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_n_audio_layer(ctx);
}

int WhisperFull::get_model_n_text_ctx() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_n_text_ctx(ctx);
}

int WhisperFull::get_model_n_text_state() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_n_text_state(ctx);
}

int WhisperFull::get_model_n_text_head() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_n_text_head(ctx);
}

int WhisperFull::get_model_n_text_layer() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_n_text_layer(ctx);
}

int WhisperFull::get_model_n_mels() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_n_mels(ctx);
}

int WhisperFull::get_model_ftype() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_ftype(ctx);
}

int WhisperFull::get_model_type() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_model_type(ctx);
}

String WhisperFull::get_model_type_readable() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, String(), "[WhisperFull] context not initialized");
	const char *type_str = whisper_model_type_readable(ctx);
	return type_str ? String::utf8(type_str) : String();
}

/* --- language utilities --- */

int WhisperFull::get_lang_max_id() {
	return whisper_lang_max_id();
}

int WhisperFull::get_lang_id(const String &p_lang) {
	CharString lang_cs = p_lang.utf8();
	return whisper_lang_id(lang_cs.get_data());
}

String WhisperFull::get_lang_str(int p_id) {
	const char *str = whisper_lang_str(p_id);
	return str ? String::utf8(str) : String();
}

String WhisperFull::get_lang_str_full(int p_id) {
	const char *str = whisper_lang_str_full(p_id);
	return str ? String::utf8(str) : String();
}

/* --- transcription methods --- */

int WhisperFull::transcribe(const PackedFloat32Array &p_samples) {
	if (!_init_context()) {
		return -1;
	}

	whisper_full_params wparams = _build_params();

	int result = whisper_full(ctx, wparams, p_samples.ptr(), p_samples.size());

	return result;
}

int WhisperFull::transcribe_parallel(const PackedFloat32Array &p_samples, int p_n_processors) {
	if (!_init_context()) {
		return -1;
	}

	whisper_full_params wparams = _build_params();

	int result = whisper_full_parallel(ctx, wparams, p_samples.ptr(), p_samples.size(), p_n_processors);

	return result;
}

/* --- get transcription results --- */

int WhisperFull::get_segment_count() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");
	return whisper_full_n_segments(ctx);
}

Ref<WhisperSegment> WhisperFull::get_segment(int p_index) const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, Ref<WhisperSegment>(), "[WhisperFull] context not initialized");

	int n_segments = whisper_full_n_segments(ctx);
	ERR_FAIL_INDEX_V_MSG(p_index, n_segments, Ref<WhisperSegment>(), "[WhisperFull] segment index out of range");

	Ref<WhisperSegment> segment;
	segment.instantiate();

	// times are in centiseconds (1/100 sec), convert to milliseconds
	segment->set_t0(whisper_full_get_segment_t0(ctx, p_index) * 10);
	segment->set_t1(whisper_full_get_segment_t1(ctx, p_index) * 10);

	const char *text = whisper_full_get_segment_text(ctx, p_index);
	segment->set_text(text ? String::utf8(text) : String());

	segment->set_speaker_turn_next(whisper_full_get_segment_speaker_turn_next(ctx, p_index));
	segment->set_no_speech_prob(whisper_full_get_segment_no_speech_prob(ctx, p_index));

	return segment;
}

int WhisperFull::get_all_segments_native(LocalVector<Ref<WhisperSegment>> &r_segments) const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, 0, "[WhisperFull] context not initialized");

	int n_segments = whisper_full_n_segments(ctx);
	uint32_t size = r_segments.size();
	r_segments.resize(size + n_segments);
	for (int i = 0; i < n_segments; i++) {
		r_segments[size + i] = get_segment(i);
	}

	return n_segments;
}

TypedArray<WhisperSegment> WhisperFull::get_all_segments() const {
	TypedArray<WhisperSegment> segments;

	ERR_FAIL_COND_V_MSG(ctx == nullptr, segments, "[WhisperFull] context not initialized");

	int n_segments = whisper_full_n_segments(ctx);
	for (int i = 0; i < n_segments; i++) {
		segments.push_back(get_segment(i));
	}

	return segments;
}

String WhisperFull::get_full_text() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, String(), "[WhisperFull] context not initialized");

	String result;
	int n_segments = whisper_full_n_segments(ctx);

	for (int i = 0; i < n_segments; i++) {
		const char *text = whisper_full_get_segment_text(ctx, i);
		if (text) {
			result += String::utf8(text);
		}
	}

	return result;
}

int WhisperFull::get_detected_lang_id() const {
	ERR_FAIL_COND_V_MSG(ctx == nullptr, -1, "[WhisperFull] context not initialized");
	return whisper_full_lang_id(ctx);
}

String WhisperFull::get_detected_language() const {
	int lang_id = get_detected_lang_id();
	if (lang_id < 0) {
		return String();
	}
	return get_lang_str(lang_id);
}

/* --- timing information --- */

Dictionary WhisperFull::get_timings() const {
	Dictionary timings;
	ERR_FAIL_COND_V_MSG(ctx == nullptr, timings, "[WhisperFull] context not initialized");

	whisper_timings *t = whisper_get_timings(ctx);
	if (t) {
		timings["sample_ms"] = t->sample_ms;
		timings["encode_ms"] = t->encode_ms;
		timings["decode_ms"] = t->decode_ms;
		timings["batchd_ms"] = t->batchd_ms;
		timings["prompt_ms"] = t->prompt_ms;
	}

	return timings;
}

void WhisperFull::print_timings() const {
	ERR_FAIL_COND_MSG(ctx == nullptr, "[WhisperFull] context not initialized");
	whisper_print_timings(ctx);
}

void WhisperFull::reset_timings() {
	ERR_FAIL_COND_MSG(ctx == nullptr, "[WhisperFull] context not initialized");
	whisper_reset_timings(ctx);
}

/* --- system info --- */

String WhisperFull::get_system_info() {
	const char *info = whisper_print_system_info();
	return info ? String::utf8(info) : String();
}

String WhisperFull::get_version() {
	const char *version = whisper_version();
	return version ? String::utf8(version) : String();
}

int WhisperFull::get_sample_rate() {
	return WHISPER_SAMPLE_RATE;
}

// perform linear interpolation resampling + stereo to mono.
// btw, this function is just for convenience, you can use your own resampling library if you want.
PackedFloat32Array WhisperFull::convert_stereo_to_mono_16khz(int p_from_sample_rate, const PackedVector2Array &p_stereo_data) {
	PackedFloat32Array result;

	if (p_stereo_data.is_empty()) {
		return result;
	}

	float whisper_sample_rate = WHISPER_SAMPLE_RATE;
	float ratio = p_from_sample_rate / whisper_sample_rate;

	int output_size = int(p_stereo_data.size() / ratio);
	if (output_size <= 0) {
		return result;
	}

	result.resize(output_size);
	float *result_ptr = result.ptrw();
	const Vector2 *stereo_ptr = p_stereo_data.ptr();
	int stereo_size = p_stereo_data.size();

	for (int i = 0; i < output_size; i++) {
		float src_index = float(i) * ratio;
		int idx0 = int(src_index);
		int idx1 = MIN(idx0 + 1, stereo_size - 1);
		float frac = src_index - float(idx0);

		// stereo to mono (average of left and right)
		float sample0 = (stereo_ptr[idx0].x + stereo_ptr[idx0].y) * 0.5f;
		float sample1 = (stereo_ptr[idx1].x + stereo_ptr[idx1].y) * 0.5f;

		// linear interpolation
		result_ptr[i] = sample0 + (sample1 - sample0) * frac;
	}

	return result;
}