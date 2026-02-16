target("zabato_sdl2")
    set_kind("static")
    add_files("sdl2.cpp")
    add_includedirs("include")
    set_languages("c++23")
    
    add_deps("zabato")
    add_packages("libsdl2")