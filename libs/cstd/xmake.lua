target("cstd")
    set_kind("static")
    
    set_languages("c++23")
    add_includedirs("include", {public = true})
    add_files("src/*.cpp")
    add_deps("libberg")