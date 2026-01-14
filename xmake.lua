add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")

set_languages("cxx20")

if is_plat("windows") then
    add_cxxflags("/utf-8")
    add_defines("NOMINMAX")
end

add_requires("spdlog", "mio")

target("gaussian-stream")
    set_kind("binary")
    add_includedirs("include")
    add_packages("spdlog", "mio")
    add_files("src/*.cpp")