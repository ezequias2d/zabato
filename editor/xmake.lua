target("editor")
    set_kind("binary")
    add_files("src/*.cpp")
    add_files("src/windows/*.cpp")
    add_files("src/core/*.cpp")
    add_files("src/windows/inspector/*.cpp")
    add_files("src/widgets/*.cpp")
    add_includedirs("include", {public = true})
    set_languages("c++23")
    add_cxxflags("-fno-rtti")
    add_deps(
        "zabato_imgui",
        "zabato_sdl2",
        "zabato_gl",
        "cstd",
        "zabato",
        "zabato_lua",
        "zabato_assimp",
        "zabato_stb",
        "zabato_platform",
        "zabato_joltphysics"
    )

    add_packages("tinyxml2", "joltphysics")

    after_build(function(target, build_result)
        local taget_dir = target:targetdir()
        
        local target_scripts = path.join(taget_dir, "scripts")
        local source_scripts = path.join(os.projectdir(), "scripts")
        os.rm(target_scripts)
        os.cp(source_scripts, target_scripts, {symlink = true})

        local target_assets = path.join(taget_dir, "assets")
        local source_assets = path.join(os.projectdir(), "assets")
        os.rm(target_assets)
        os.cp(source_assets, target_assets, {symlink = true})
    end)