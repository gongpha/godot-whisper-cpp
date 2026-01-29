import subprocess
import os
import glob
import shutil

thirdparty_dir = "thirdparty/whisper.cpp/"

def _process_env(self, env, sources):
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

    # vulkan support
    _setup_vulkan(self, env, sources)


def _setup_vulkan(self, env, sources):
    """Setup Vulkan backend support"""
    from SCons.Script import Environment, ARGUMENTS

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
                potential_paths = [
                    "/usr/bin/glslc",
                    "/usr/local/bin/glslc",
                    os.path.expanduser("~/.local/bin/glslc"),
                ]
                for p in potential_paths:
                    if os.path.exists(p):
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

        shader_tool = tool_env.Program(
            target=shader_gen_exe,
            source=[gen_src],
        )

        # check if we need to generate shaders (header doesn't exist)
        need_generate = not os.path.exists(target_hpp)

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

        # if shaders don't exist and the tool already exists, generate now
        if need_generate and os.path.exists(shader_gen_exe):
            if not generate_all_shaders(shader_gen_exe):
                from SCons.Script import Exit
                Exit(1)

        # action for Command to run after tool is built
        def run_shader_gen_action(target, source, env):
            tool_path = str(source[0])
            if not generate_all_shaders(tool_path):
                return 1
            # touch the stamp file
            with open(str(target[0]), 'w') as f:
                f.write("generated")
            return 0

        # create a stamp file target to trigger shader generation
        shader_gen_stamp = os.path.join(gen_output_dir, ".shader_gen_stamp")

        # only run Command if we still need to generate
        if need_generate:
            from SCons.Script import Command, Depends
            shader_gen_cmd = Command(
                shader_gen_stamp,
                shader_tool,
                run_shader_gen_action
            )
            # make sure sources depend on shader generation
            Depends(sources, shader_gen_cmd)

        # add ggml-vulkan.cpp
        sources.append(vulkan_dir + "ggml-vulkan.cpp")

        # add generated cpp files
        for shader_file in shader_comp_files:
            shader_name = os.path.basename(shader_file)
            target_cpp = os.path.join(gen_output_dir, shader_name + ".cpp")
            sources.append(target_cpp)

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
            env.Append(LIBPATH=[os.path.join(vulkan_sdk, "Lib")])
        env.Append(LIBS=["vulkan-1"])
    elif env["platform"] == "macos":
        # (didnt test this yet)
        env.Append(LIBS=["vulkan"])
        env.Append(FRAMEWORKS=["MoltenVK"])
    else:
        env.Append(LIBS=["vulkan"])