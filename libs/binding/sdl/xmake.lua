add_requires("libsdl2")

target("zabato_sdl2")
    set_kind("shared")
    add_files("sdl2.cpp")
    add_includedirs("include")
    
    add_deps("zabato")
    add_packages("libsdl2")