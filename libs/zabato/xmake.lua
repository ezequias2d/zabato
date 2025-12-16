add_requires("tinyxml2")

target("zabato")
    set_languages("c++23")
    set_kind("shared")
    add_includedirs("include", {public = true})
    add_files("src/*.cpp")
    add_deps("berg", "cstd")
    add_packages("tinyxml2")

    if is_kind("shared") or is_kind("static") or is_kind("binary") then
        if is_plat("windows") and is_toolset("msvc") then
            add_cxxflags("/GR-")
        else
            add_cxxflags("-fno-rtti")
        end
    end

target("zabato_header")
    set_kind("headeronly")
    add_includedirs("include", {public = true})
