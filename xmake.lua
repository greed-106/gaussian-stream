add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")

set_languages("cxx20")

if is_plat("windows") then
    add_cxxflags("/utf-8")
    add_defines("NOMINMAX")
end

if is_plat("windows") then
    add_vectorexts("avx2")
elseif is_plat("linux") then 
    add_cxxflags("-mbmi2")
end

add_requires("spdlog", "mio", "morton-nd")

target("gaussian-stream")
    set_kind("binary")
    add_includedirs("include")
    add_packages("spdlog", "mio", "morton-nd")
    add_files("src/*.cpp")