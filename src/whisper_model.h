#pragma once

#ifdef _GDEXTENSION
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
using namespace godot;
#else
#include "core/io/resource.h"
#endif

#include <whisper.h>


class WhisperModel : public Resource {
    GDCLASS(WhisperModel, Resource);

    String path;

protected:
    static void _bind_methods();
    
public:
    void set_bin_path(const String &p_path) { path = p_path; }
    String get_bin_path() const { return path; }

    WhisperModel();
    ~WhisperModel();
};

class ResourceFormatLoaderWhisperModel : public ResourceFormatLoader {
    GDCLASS(ResourceFormatLoaderWhisperModel, ResourceFormatLoader);

protected:
    static void _bind_methods();

public:
#ifdef _GDEXTENSION
    virtual Variant _load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const;
    virtual PackedStringArray _get_recognized_extensions() const override;
    virtual bool _handles_type(const StringName &type) const override;
	virtual String _get_resource_type(const String &p_path) const override;
#else
    virtual Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float p_priority = 0.0);
    virtual void get_recognized_extensions(List<String> *r_extensions) const;
    virtual bool handles_type(const String &p_type) const;
    virtual String get_resource_type(const String &p_path) const;
#endif
   
};