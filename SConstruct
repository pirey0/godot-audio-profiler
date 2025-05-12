#!/usr/bin/env python
import os
import sys
import shutil

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

libname = "audio_profiler"
suffix = env["suffix"]
shlibsuffix = env["SHLIBSUFFIX"]

file = f"{libname}{suffix}{shlibsuffix}"
output_path = os.path.join("bin", file)

# Build the shared library into bin/
library = env.SharedLibrary(output_path, source=sources)

# copy to demo dir for quicker testing
def copy_bin_dir(target, source, env):
    src_dir = "bin"
    dst_dir = "demo/addons/exqueesite_audio_profiler/"
    if os.path.exists(dst_dir):
        shutil.rmtree(dst_dir)
    shutil.copytree(src_dir, dst_dir)
    print(f"Copied {src_dir} -> {dst_dir}")

env.AddPostAction(library, copy_bin_dir)

Default(library)
