target("zabato_imgui")
    set_kind("static")
    add_files("imgui.cpp")
    add_defines("IMGUI_ENABLE_FREETYPE", {public = true})
    add_includedirs("include", {public = true})
    set_languages("c++23")
    
    add_deps("zabato", "imgui")
    add_packages("tinyxml2")