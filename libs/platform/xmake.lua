add_requires("nativefiledialog-extended")

target("zabato_platform")
    set_languages("c++23")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_files("src/*.cpp")
    add_deps("libberg", "cstd")
    add_packages("tinyxml2", "nativefiledialog-extended")

    if is_kind("shared") or is_kind("static") or is_kind("binary") then
        if is_plat("windows") then
            add_cxxflags("/GR-")
        else
            add_cxxflags("-fno-rtti")
        end
    end
