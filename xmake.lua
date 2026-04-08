-- xmake build for WinUI3 C++/WinRT desktop app
-- Uses xmake rules + Python orchestrator for code generation
-- No MSBuild dependency; MSVC + NuGet packages + manual tool invocation
--
-- Build pipeline:
--   [Rule: winui3.codegen] → Python orchestrator (8 phases)
--   → MSVC compilation → Linking → [Rule: winui3.postbuild]

set_project("xmake_demo")
set_version("1.0.0")
set_languages("cxx20")

-- Include custom rules
includes("rules/winui3_rules.lua")

-- NuGet package paths (using USERPROFILE for portability)
-- WinAppSDK 1.8+ splits into sub-packages (Foundation, WinUI, InteractiveExperiences)
local nuget      = (os.getenv("USERPROFILE") or ""):gsub("\\", "/") .. "/.nuget/packages"
local foundation = nuget .. "/microsoft.windowsappsdk.foundation/1.8.260415000"
local winui      = nuget .. "/microsoft.windowsappsdk.winui/1.8.260415005"
local ixp        = nuget .. "/microsoft.windowsappsdk.interactiveexperiences/1.8.260415001"
local runtime    = nuget .. "/microsoft.windowsappsdk.runtime/1.8.260416003"
local wil        = nuget .. "/microsoft.windows.implementationlibrary/1.0.260126.7"

target("xmake_demo")
    set_kind("binary")
    set_filename("xmake_demo.exe")
    set_policy("build.across_targets_in_parallel", false)
    
    -- Apply WinUI3 build rules
    add_rules("winui3.codegen", "winui3.postbuild", "winui3.run")
    
    -- Compiler flags for C++/WinRT
    add_cxflags("/EHsc", "/bigobj", "/await:strict",
                "/DNOMINMAX",
                "/DUNICODE", "/D_UNICODE")
    
    -- Include directories (sub-packages + WIL)
    add_includedirs(runtime .. "/include")
    add_includedirs(foundation .. "/include")
    add_includedirs(winui .. "/include")
    add_includedirs(ixp .. "/include")
    add_includedirs(wil .. "/include")
    add_includedirs("src")
    
    -- Link libraries
    add_links(path.translate(foundation .. "/lib/native/x64/Microsoft.WindowsAppRuntime.Bootstrap.lib"))
    add_links("windowsapp")
    add_ldflags("/SUBSYSTEM:WINDOWS")
    
    -- Source files
    add_files("src/main.cpp")
    add_files("src/App.xaml.cpp")
    add_files("src/MainWindow.xaml.cpp")
    add_files("src/XamlMetaDataProvider.cpp")
    add_files("src/app.manifest")
