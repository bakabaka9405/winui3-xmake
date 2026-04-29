-- xmake build for WinUI3 C++/WinRT desktop app
-- Root configuration: defines shared NuGet paths, includes rules, and auto-discovers
-- all demo targets via includes("demo/*/xmake.lua").
-- Build individual demos:  xmake build hello_demo   /  xmake build notepad_demo
-- No MSBuild dependency; MSVC + NuGet packages + manual tool invocation

set_project("winui3_demos")
set_version("1.0.0")
set_languages("cxx20")

-- Include custom rules
includes("rules/winui3_rules.lua")

-- NuGet package paths (shared global variables for use by per-demo xmake.lua)
NUGET_ROOT       = (os.getenv("USERPROFILE") or ""):gsub("\\", "/") .. "/.nuget/packages"
NUGET_FOUNDATION = NUGET_ROOT .. "/microsoft.windowsappsdk.foundation/1.8.260415000"
NUGET_WINUI      = NUGET_ROOT .. "/microsoft.windowsappsdk.winui/1.8.260415005"
NUGET_IXP        = NUGET_ROOT .. "/microsoft.windowsappsdk.interactiveexperiences/1.8.260415001"
NUGET_RUNTIME    = NUGET_ROOT .. "/microsoft.windowsappsdk.runtime/1.8.260416003"
NUGET_WIL        = NUGET_ROOT .. "/microsoft.windows.implementationlibrary/1.0.260126.7"

-- Include all demo targets (each demo/<name>/xmake.lua defines its own target)
includes("demo/*/xmake.lua")

