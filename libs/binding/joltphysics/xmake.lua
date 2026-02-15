add_requires("joltphysics")

target("zabato_joltphysics")
    set_kind("static")
    add_files("src/*.cpp")
    add_includedirs("include", {public = true})

    add_deps("zabato")
    add_cxxflags("-fno-rtti")
    set_languages("c++23")
    add_packages("tinyxml2", "joltphysics")