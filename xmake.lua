add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

add_requires("lua", "tinyxml2", "freetype", "assimp", "stb", "libsdl2")

includes("ext")
includes("libs")
includes("editor")

set_rundir("./assets")