target("zabato_imgui")
    set_kind("static")
    add_files("imgui.cpp")
    add_includedirs("include", {public = true})
    set_languages("c++23")
    
    add_deps("zabato", "imgui")