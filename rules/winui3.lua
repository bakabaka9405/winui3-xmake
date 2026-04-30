-- xmake rule for WinUI3 C++/WinRT build pipeline
--
-- This single rule wraps the entire WinUI3 application lifecycle:
--   1. Target configuration  (kind, filename, output dir, policy)
--   2. Compiler flags        (C++20, WinRT, Unicode)
--   3. NuGet include paths   (WindowsAppSDK, WinUI, WIL)
--   4. Link libraries        (Bootstrap, WindowsApp)
--   5. Code generation       (XAML + IDL → WinMD → projection via Python)
--   6. Post-build            (copy Bootstrap DLL, .xbf, .pri to output)
--   7. Run                   (execute from output directory)

local NUGET_BASE = (os.getenv("USERPROFILE") or ""):gsub("\\", "/") .. "/.nuget/packages"

local NUGET_RUNTIME    = NUGET_BASE .. "/microsoft.windowsappsdk.runtime/1.8.260416003"
local NUGET_FOUNDATION = NUGET_BASE .. "/microsoft.windowsappsdk.foundation/1.8.260415000"
local NUGET_WINUI      = NUGET_BASE .. "/microsoft.windowsappsdk.winui/1.8.260415005"
local NUGET_IXP        = NUGET_BASE .. "/microsoft.windowsappsdk.interactiveexperiences/1.8.260415001"
local NUGET_WIL        = NUGET_BASE .. "/microsoft.windows.implementationlibrary/1.0.260126.7"

rule("winui3.app")
    -- ── on_load: configure target identity, search paths, compiler & linker flags ──
    on_load(function (target)
        -- ── Target identity ──
        target:set("kind", "binary")
        target:set("filename", target:name() .. ".exe")
        target:set("targetdir", path.join("$(builddir)", "$(host)", "$(mode)", "$(arch)", target:name()))
        target:set("policy", "build.across_targets_in_parallel", false)

        -- ── Per-target values with defaults ──
        if not target:values("winui3.root_dir") then
            target:set("values", "winui3.root_dir", os.projectdir())
        end

        -- ── Generated output include directories ──
        target:add("includedirs", path.join(target:targetdir(), "generated"))
        target:add("includedirs", path.join(target:targetdir(), "generated", "sources"))
        target:add("includedirs", path.join(path.directory(target:targetdir()), "shared", "generated"))

        -- ── NuGet include directories ──
        target:add("includedirs", NUGET_RUNTIME .. "/include")
        target:add("includedirs", NUGET_FOUNDATION .. "/include")
        target:add("includedirs", NUGET_WINUI .. "/include")
        target:add("includedirs", NUGET_IXP .. "/include")
        target:add("includedirs", NUGET_WIL .. "/include")

        -- ── Per-target src include directory (derived from winui3.src_dir) ──
        local src_dir = target:values("winui3.src_dir")
        if src_dir then
            target:add("includedirs", src_dir)
        end

        -- ── Compiler flags ──
        target:add("cxflags", "/EHsc", "/bigobj", "/await:strict", "/utf-8",
                    "/DNOMINMAX", "/DWIN32_LEAN_AND_MEAN", "/DUNICODE", "/D_UNICODE")

        -- ── Link libraries ──
        target:add("links", path.translate(NUGET_FOUNDATION .. "/lib/native/x64/Microsoft.WindowsAppRuntime.Bootstrap.lib"))
        target:add("links", "windowsapp")
        target:add("ldflags", "/SUBSYSTEM:WINDOWS")
    end)

    -- ── before_build: code generation (Python orchestrator) ──
    before_build(function (target)
        import("core.project.depend")

        local namespace = target:values("winui3.namespace") or "xmake_demo"
        local src_dir   = target:values("winui3.src_dir") or path.join(os.projectdir(), "src")
        local root_dir  = target:values("winui3.root_dir") or os.projectdir()
        local build_dir = target:targetdir()
        local shared_projection_dir = path.join(path.directory(build_dir), "shared", "generated")

        depend.on_changed(function ()
            print("")
            print("=== WinUI3 Code Generation ===")
            local py     = "python"
            local script = path.join(root_dir, "scripts/build_winui3.py")
            local args   = {
                path.translate(script),
                "--build-dir", path.translate(build_dir),
                "--project-dir", path.translate(root_dir),
                "--namespace", namespace,
                "--src-dir", path.translate(src_dir),
                "--shared-projection-dir", path.translate(shared_projection_dir),
            }
            print("  " .. py .. " " .. table.concat(args, " "))
            os.runv(py, args)
            print("=== Code Generation Complete ===\n")
        end, {
            dependfile = path.join(build_dir, "generated", ".codegen_stamp"),
            files = table.join(
                os.files(path.join(src_dir, "**.xaml")),
                os.files(path.join(src_dir, "**.xaml.h")),
                os.files(path.join(src_dir, "**.idl")),
                { path.join(root_dir, "scripts", "build_winui3.py"), path.join(root_dir, "rules", "winui3_rules.lua") }
            ),
        })
    end)

    -- ── after_build: copy runtime files to output ──
    after_build(function (target)
        local outdir = target:targetdir()

        -- Copy Bootstrap DLL
        local dll_src = (NUGET_FOUNDATION .. "/runtimes/win-x64/native/Microsoft.WindowsAppRuntime.Bootstrap.dll"):gsub("/", "\\")
        local dll_dst = path.join(outdir, "Microsoft.WindowsAppRuntime.Bootstrap.dll")
        os.cp(dll_src, dll_dst)

        -- Copy .xbf files from generated/ to output root
        local gendir = path.join(outdir, "generated")
        for _, xbf in ipairs(os.files(gendir .. "/*.xbf")) do
            local dst = path.join(outdir, path.filename(xbf))
            os.cp(xbf, dst)
        end

        -- Copy .pri from generated/ to output root (MRT resource index)
        local pri_src = path.join(gendir, "resources.pri")
        if os.isfile(pri_src) then
            local pri_dst = path.join(outdir, "resources.pri")
            os.cp(pri_src, pri_dst)
            local named_dst = path.join(outdir, target:name() .. ".pri")
            os.cp(pri_src, named_dst)
        end

        print("[Post-build] Output: " .. path.translate(target:targetfile()))
    end)

    -- ── on_run: execute from output directory ──
    on_run(function (target)
        os.runv(path.translate(target:targetfile()), {}, {curdir = target:targetdir()})
    end)
