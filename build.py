import subprocess
import os
import glob
import shutil

from SCons.Variables import BoolVariable

thirdparty_dir = "thirdparty/whisper.cpp/"

def _setup_options(opts):
    opts.Add(BoolVariable("use_vulkan", "Enable Vulkan GPU acceleration", False))

def _process_env(self, env, sources, is_gdextension):
    if env["platform"] == "windows":
        is_msvc = "mingw" not in env["TOOLS"]
    else :
        is_msvc = False

    # here we go again
    if is_msvc:
        env.Append(CXXFLAGS=["/EHsc"])
    else:
        env.Append(CXXFLAGS=["-fexceptions"])

    # retrieve git info
    try:
        build_number = int(subprocess.check_output(
            ["git", "rev-list", "--count", "HEAD"],
            cwd="thirdparty/llama",
            universal_newlines=True
        ).strip())
    except Exception:
        build_number = 0

    try:
        commit = subprocess.check_output(
                ["git", "rev-parse", "--short=7", "HEAD"],
                cwd="thirdparty/llama",
                universal_newlines=True
        ).strip()
    except Exception:
        commit = "unknown"
	
    # setup ggml sources and includes

    env.Append(CPPDEFINES=[
        'GGML_VERSION="\\\"' + "b" + str(build_number) + '\\\""',
        'GGML_COMMIT="\\\"' + commit + '\\\""',
        'WHISPER_VERSION="\\\"' + "1.8.3" + '\\\""',

        'GGML_USE_CPU'
    ])
    
    env.Append(CPPPATH=[thirdparty_dir,
        thirdparty_dir + "include",
        thirdparty_dir + "ggml/include",
        thirdparty_dir + "ggml/src",

        thirdparty_dir + "ggml/src/ggml-cpu",
    ])
    
    sources.extend(self.Glob("src/*.c"))

    sources.extend([
        thirdparty_dir + "ggml/src/ggml-backend.cpp",
        thirdparty_dir + "ggml/src/ggml-backend-reg.cpp",
        thirdparty_dir + "ggml/src/ggml-opt.cpp",
        thirdparty_dir + "ggml/src/ggml-threading.cpp",
        thirdparty_dir + "ggml/src/gguf.cpp",
    ])

    sources.extend([
        thirdparty_dir + "ggml/src/ggml-cpu/binary-ops.cpp",
        thirdparty_dir + "ggml/src/ggml-cpu/ggml-cpu.cpp",
        thirdparty_dir + "ggml/src/ggml-cpu/hbm.cpp",
        thirdparty_dir + "ggml/src/ggml-cpu/ops.cpp",
        thirdparty_dir + "ggml/src/ggml-cpu/repack.cpp",
        thirdparty_dir + "ggml/src/ggml-cpu/traits.cpp",
        thirdparty_dir + "ggml/src/ggml-cpu/unary-ops.cpp",
        thirdparty_dir + "ggml/src/ggml-cpu/vec.cpp",
        thirdparty_dir + "ggml/src/ggml-cpu/quants.c",

    ])

    sources.extend(self.Glob(thirdparty_dir + "ggml/src/*.c"))
    sources.extend(self.Glob(thirdparty_dir + "src/*.cpp"))

    # Windows-specific libraries for ggml-cpu (uses Registry functions)
    if env["platform"] == "windows":
        env.Append(LIBS=["Advapi32"])

    if env["use_vulkan"]:
        # vulkan support
        _setup_vulkan(self, env, sources, is_gdextension)


def _setup_vulkan(self, env, sources, is_gdextension):
    """Setup Vulkan backend support"""
    from SCons.Script import Environment, ARGUMENTS, Command, Depends

    print("Enabling Vulkan support for whisper.cpp")

    env.Append(CPPDEFINES=['GGML_USE_VULKAN'])

    vulkan_dir = thirdparty_dir + "ggml/src/ggml-vulkan/"
    vulkan_shaders_dir = vulkan_dir + "vulkan-shaders/"

    # check if pre-generated shaders exist
    pregenerated_cpp = vulkan_dir + "ggml-vulkan-shaders.cpp"
    pregenerated_hpp = vulkan_dir + "ggml-vulkan-shaders.hpp"
    has_pregenerated = os.path.exists(pregenerated_cpp) and os.path.exists(pregenerated_hpp)

    # regenerate shaders if requested or if pre-generated files don't exist
    regenerate_shaders = ARGUMENTS.get('regenerate_vulkan_shaders', 'no') == 'yes' or not has_pregenerated

    if regenerate_shaders:
        if not has_pregenerated:
            print("Pre-generated Vulkan shaders not found, will generate them...")
        else:
            print("Regenerating Vulkan shaders as requested...")

        gen_src = vulkan_shaders_dir + "vulkan-shaders-gen.cpp"

        # create a host tool environment for building the shader generator
        is_msvc = env["platform"] == "windows" and "mingw" not in env["TOOLS"]
        tool_env = Environment(tools=['default'], ENV=os.environ.copy())
        if is_msvc:
            tool_env.Append(CXXFLAGS=['/std:c++17', '/EHsc'])
        else:
            tool_env.Append(CXXFLAGS=['-std=c++17', '-pthread'])
            tool_env.Append(LINKFLAGS=['-pthread'])

        # find glslc compiler (from Vulkan SDK)
        glslc_path = shutil.which("glslc")
        if glslc_path is None:
            # find manually
            if env["platform"] == "windows":
                vulkan_sdk = os.environ.get("VULKAN_SDK", "C:/VulkanSDK")
                if os.path.exists(vulkan_sdk):
                    for d in os.listdir(vulkan_sdk):
                        potential = os.path.join(vulkan_sdk, d, "Bin", "glslc.exe")
                        if os.path.exists(potential):
                            glslc_path = potential
                            break
            else:
                # check VULKAN_SDK environment variable first
                vulkan_sdk = os.environ.get("VULKAN_SDK", "")
                potential_paths = [
                    os.path.join(vulkan_sdk, "bin", "glslc") if vulkan_sdk else "",
                    "/usr/bin/glslc",
                    "/usr/local/bin/glslc",
                    os.path.expanduser("~/.local/bin/glslc"),
                ]
                for p in potential_paths:
                    if p and os.path.exists(p):
                        glslc_path = p
                        break

        if glslc_path is None or not os.path.exists(glslc_path):
            print("ERROR: glslc not found")
            from SCons.Script import Exit
            Exit(1)

        #print("Using glslc: " + glslc_path)

        # output paths
        gen_output_dir = os.path.abspath("bin/gen/vulkan-shaders")
        spv_output_dir = os.path.join(gen_output_dir, "spv")
        target_hpp = os.path.join(gen_output_dir, "ggml-vulkan-shaders.hpp")
        shaders_cwd = os.path.abspath(vulkan_shaders_dir)

        # get all shader source files
        shader_comp_files = glob.glob(os.path.join(shaders_cwd, "*.comp"))

        # ensure output directory exists
        os.makedirs(spv_output_dir, exist_ok=True)

        # add generated header include path
        env.Append(CPPPATH=[gen_output_dir])

        # build the shader generator tool
        shader_gen_exe = os.path.abspath("bin/vulkan-shaders-gen")
        if env["platform"] == "windows":
            shader_gen_exe += ".exe"

        # check if we need to generate shaders (header doesn't exist or any cpp missing)
        def check_need_generate():
            if not os.path.exists(target_hpp):
                return True
            for shader_file in shader_comp_files:
                shader_name = os.path.basename(shader_file)
                target_cpp = os.path.join(gen_output_dir, shader_name + ".cpp")
                if not os.path.exists(target_cpp):
                    return True
            return False

        need_generate = check_need_generate()

        # function to generate all shaders
        def generate_all_shaders(tool_path):
            print("Generating Vulkan shaders...")
            # first generate the header
            cmd = [tool_path, "--output-dir", spv_output_dir, "--target-hpp", target_hpp]
            print("Running: " + " ".join(cmd))
            result = subprocess.run(cmd, cwd=shaders_cwd)
            if result.returncode != 0:
                print("ERROR: Failed to generate shader header")
                return False

            # then generate cpp files for each shader
            for shader_file in shader_comp_files:
                shader_name = os.path.basename(shader_file)
                target_cpp = os.path.join(gen_output_dir, shader_name + ".cpp")
                cmd = [
                    tool_path,
                    "--glslc", glslc_path,
                    "--source", shader_file,
                    "--output-dir", spv_output_dir,
                    "--target-hpp", target_hpp,
                    "--target-cpp", target_cpp
                ]
                print("Compiling shader: " + shader_name)
                result = subprocess.run(cmd, cwd=shaders_cwd)
                if result.returncode != 0:
                    print("ERROR: Failed to compile shader: " + shader_name)
                    return False
            print("Vulkan shaders generated successfully!")
            return True

        # Build the shader generator tool if it doesn't exist
        if need_generate and not os.path.exists(shader_gen_exe):
            print("Building vulkan-shaders-gen tool...")
            # We need to build the tool synchronously before SCons evaluates the build graph
            # because the generated cpp files must exist for SCons to compile them
            shader_tool = tool_env.Program(
                target=shader_gen_exe,
                source=[gen_src],
            )
            # Force the tool to build now by executing SCons for just this target
            # We use subprocess to invoke scons for this specific tool
            import sys
            
            # Compile the shader generator manually using the tool_env settings
            is_msvc = env["platform"] == "windows" and "mingw" not in env["TOOLS"]
            if is_msvc:
                compile_cmd = ["cl", "/std:c++17", "/EHsc", gen_src, "/Fe:" + shader_gen_exe]
            else:
                compile_cmd = ["g++", "-o", shader_gen_exe, "-std=c++17", "-pthread", gen_src]
            
            print("Compiling shader generator: " + " ".join(compile_cmd))
            result = subprocess.run(compile_cmd, cwd=os.path.dirname(os.path.abspath(__file__)))
            if result.returncode != 0:
                print("ERROR: Failed to compile vulkan-shaders-gen")
                from SCons.Script import Exit
                Exit(1)
        
        # Generate shaders if needed (tool should exist now)
        if need_generate:
            if not os.path.exists(shader_gen_exe):
                print("ERROR: vulkan-shaders-gen not found at: " + shader_gen_exe)
                from SCons.Script import Exit
                Exit(1)
            if not generate_all_shaders(shader_gen_exe):
                from SCons.Script import Exit
                Exit(1)

        # add ggml-vulkan.cpp
        if is_gdextension:
            vulkan_object = self.SharedObject
        else:
            vulkan_object = self.Object

        vulkan_cpp = vulkan_object(vulkan_dir + "ggml-vulkan.cpp")
        sources.append(vulkan_cpp)

        # add generated cpp files (they now exist after synchronous generation)
        for shader_file in shader_comp_files:
            shader_name = os.path.basename(shader_file)
            target_cpp = os.path.join(gen_output_dir, shader_name + ".cpp")
            shader_obj = vulkan_object(target_cpp)
            sources.append(shader_obj)

    else:
        # use pre-generated shader files
        print("Using pre-generated Vulkan shaders")
        env.Append(CPPPATH=[vulkan_dir])
        sources.append(vulkan_dir + "ggml-vulkan.cpp")
        sources.append(pregenerated_cpp)

    # link with Vulkan library
    if env["platform"] == "windows":
        vulkan_sdk = os.environ.get("VULKAN_SDK", "")
        if vulkan_sdk:
            env.Append(CPPPATH=[os.path.join(vulkan_sdk, "Include")])
            env.Append(LIBPATH=[os.path.join(vulkan_sdk, "Lib")])
        env.Append(LIBS=["vulkan-1"])
    elif env["platform"] == "macos":
        # (didnt test this yet)
        env.Append(LIBS=["vulkan"])
        env.Append(FRAMEWORKS=["MoltenVK"])
    else:
        env.Append(LIBS=["vulkan"])