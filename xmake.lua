-- xmake build for WinUI3 C++/WinRT desktop app
-- Root configuration: includes rules, auto-discovers demo targets.
-- Build individual demos:  xmake build hello_demo  /  xmake build notepad_demo
-- No MSBuild dependency; MSVC + NuGet packages + manual tool invocation


set_project("winui3_demos")
add_rules("mode.debug", "mode.release")
set_version("1.0.0")
set_languages("cxx20")

-- Include custom rules (winui3.app handles all WinUI3 build pipeline;
-- demo.common handles project-specific shared config like common/ directory)
includes("rules/winui3.lua")
includes("rules/demo.lua")

-- Include all demo targets (each demo/<name>/xmake.lua defines its own target)
includes("demo/*/xmake.lua")
