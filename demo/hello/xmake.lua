-- Hello World WinUI3 demo
local SCRIPT_DIR = os.scriptdir()

target("hello_demo")
    set_kind("binary")
    set_filename("hello_demo.exe")
    set_policy("build.across_targets_in_parallel", false)

    -- Per-target output isolation: exe and runtime files go to their own directory
    set_targetdir("$(builddir)/$(host)/$(mode)/$(arch)/hello_demo")

    -- WinUI3 codegen configuration
    add_rules("winui3.codegen", "winui3.postbuild", "winui3.run")
    set_values("winui3.namespace", "hello")
    set_values("winui3.src_dir", path.join(SCRIPT_DIR, "src"))
    set_values("winui3.root_dir", os.projectdir())

    -- Compiler flags
    add_cxflags("/EHsc", "/bigobj", "/await:strict", "/std:c++20",
                "/DNOMINMAX", "/DUNICODE", "/D_UNICODE")

    -- Include directories
    add_includedirs(NUGET_RUNTIME .. "/include")
    add_includedirs(NUGET_FOUNDATION .. "/include")
    add_includedirs(NUGET_WINUI .. "/include")
    add_includedirs(NUGET_IXP .. "/include")
    add_includedirs(NUGET_WIL .. "/include")
    add_includedirs(path.join(SCRIPT_DIR, "src"))
    add_includedirs(path.join(SCRIPT_DIR, "..", "..", "common"))

    -- Link libraries
    add_links(path.translate(NUGET_FOUNDATION .. "/lib/native/x64/Microsoft.WindowsAppRuntime.Bootstrap.lib"))
    add_links("windowsapp")
    add_ldflags("/SUBSYSTEM:WINDOWS")

    -- Source files
    add_files(path.join(SCRIPT_DIR, "src", "**.cpp"))
    add_files(path.join(SCRIPT_DIR, "..", "..", "common", "**.cpp"))
    add_files(path.join(SCRIPT_DIR, "..", "..", "common", "**.manifest"))
