set_project("winui3_demos")
add_moduledirs("xmake/modules")
includes("xmake/plugins/*.lua")
includes("xmake/rules/*.lua")
add_rules("mode.debug", "mode.release", "mode.dist")
set_version("1.0.0")
set_languages("cxx20")

option("winui3_xaml_compiler_path")
    set_default("")
    set_showmenu(true)
    set_description("XAML 编译器路径覆盖")
option_end()

includes("demo/*/xmake.lua")

