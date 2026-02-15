add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

add_requires("tinyxml2", "freetype", "assimp", "stb", "libsdl2", "joltphysics")

includes("ext")
includes("libs")
includes("editor")

set_rundir("./assets")
