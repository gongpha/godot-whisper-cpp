#pragma once

#ifdef _GDEXTENSION
#include <godot_cpp/godot.hpp>
using namespace godot;
#else
#include "modules/register_module_types.h"
#endif

void initialize_whisper_module(ModuleInitializationLevel p_level);
void uninitialize_whisper_module(ModuleInitializationLevel p_level);