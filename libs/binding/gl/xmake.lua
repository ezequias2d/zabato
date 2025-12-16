target("zabato_gl")
    set_kind("static")
    set_languages("c++23")
    add_files("gl.cpp")
    add_includedirs("include", {public = true})
    add_deps("zabato")
    
    if not is_plat("wasm") then
        add_deps("glad")
    else 
        add_deps("gl4es")
    end
