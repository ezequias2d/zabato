target("berg")
    set_kind("shared")
    set_languages("c99")
    add_files("berg.c")
    add_includedirs("include", {public = true})
    set_basename("berg")

target("cli_berg")
    set_kind("binary")
    set_languages("c++23")
    add_files("main.c")
    
    add_deps("berg")
    set_basename("berg")