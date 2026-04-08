add_rules("mode.debug", "mode.release")
set_languages("cxx20")
set_allowedplats("windows")
set_allowedarchs("x64")

includes("xmake/rules/winui3.lua")
includes("xmake/targets/test.lua")
