#include "whisper_model.h"

WhisperModel::WhisperModel() {
}

WhisperModel::~WhisperModel() {
}

void ResourceFormatLoaderWhisperModel::_bind_methods() {
}

void WhisperModel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_bin_path", "path"), &WhisperModel::set_bin_path);
    ClassDB::bind_method(D_METHOD("get_bin_path"), &WhisperModel::get_bin_path);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "bin_path"), "set_bin_path", "get_bin_path");
}

//

#ifdef _GDEXTENSION

Variant ResourceFormatLoaderWhisperModel::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {
	Ref<WhisperModel> model;
    model.instantiate();
    model->set_bin_path(p_path);
    return model;
}

PackedStringArray ResourceFormatLoaderWhisperModel::_get_recognized_extensions() const {
    PackedStringArray exts;
    exts.push_back("bin");
    return exts;
}

bool ResourceFormatLoaderWhisperModel::_handles_type(const StringName &type) const {
	return ClassDB::is_parent_class(type, "WhisperModel");
}
String ResourceFormatLoaderWhisperModel::_get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "bin") {
		return "WhisperModel";
	}
	return String();
}

#else

Ref<Resource> ResourceFormatLoaderWhisperModel::load(
    const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float p_priority
) {
    Ref<WhisperModel> model;
    model.instantiate();
    model->set_bin_path(p_path);
    if (r_error) {
        *r_error = OK;
    }
    return model;
}

void ResourceFormatLoaderWhisperModel::get_recognized_extensions(List<String> *r_extensions) const {
    r_extensions->push_back("bin");
}

bool ResourceFormatLoaderWhisperModel::handles_type(const String &p_type) const {
    return ClassDB::is_parent_class(p_type, "WhisperModel");
}

String ResourceFormatLoaderWhisperModel::get_resource_type(const String &p_path) const {
    if (p_path.get_extension().to_lower() == "bin") {
		return "WhisperModel";
	}
	return String();
}

#endif