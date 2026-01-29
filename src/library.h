#ifdef _GDEXTENSION
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
using namespace godot;
#else
#include "modules/register_module_types.h"
#include "core/io/resource_loader.h"
#endif

#include "whisper_model.h"
#include "whisper_full.h"
#include "whisper_microphone_transcriber.h"

static Ref<ResourceFormatLoaderWhisperModel> whisper_model_resource_loader;

void initialize_library(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

    //
    GDREGISTER_CLASS(WhisperModel);
    GDREGISTER_CLASS(ResourceFormatLoaderWhisperModel);
    GDREGISTER_CLASS(WhisperSegment);
    GDREGISTER_CLASS(WhisperFull);
    GDREGISTER_CLASS(WhisperMicrophoneTranscriber);

    whisper_model_resource_loader.instantiate();
    ResourceLoader::get_singleton()->add_resource_format_loader(whisper_model_resource_loader);
}

void uninitialize_library(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

    //
    ResourceLoader::get_singleton()->remove_resource_format_loader(whisper_model_resource_loader);
    whisper_model_resource_loader.unref();
}