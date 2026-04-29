-- xmake rules for WinUI3 C++/WinRT build pipeline
-- These rules demonstrate the xmake rule() mechanism for custom build steps
-- Complex logic (XamlCompiler JSON, WinMD discovery) is handled by Python scripts

-- ── Rule: WinUI3 Code Generation ───────────────────────────────
-- This rule wraps the Python orchestrator as a before_build step.
-- It handles all 8 phases of WinUI3 code generation.
rule("winui3.codegen")
    on_load(function (target)
        -- Add generated output directories to include paths (per-target via set_targetdir)
        target:add("includedirs", path.join(target:targetdir(), "generated"))
        target:add("includedirs", path.join(target:targetdir(), "generated", "sources"))
    end)
    before_build(function (target)
        import("core.project.depend")
        
        -- Resolve target-specific values with defaults for single-target compatibility
        local namespace = target:values("winui3.namespace") or "xmake_demo"
        local src_dir = target:values("winui3.src_dir") or path.join(os.projectdir(), "src")
        local root_dir = target:values("winui3.root_dir") or os.projectdir()
        local build_dir = target:targetdir()
        
        -- Only run codegen if source files changed
        depend.on_changed(function ()
            print("")
            print("=== WinUI3 Code Generation ===")
            local py = "python"
            local script = path.join(root_dir, "scripts/build_winui3.py")
            local cmd = py .. ' "' .. path.translate(script) .. '" --build-dir "' .. path.translate(build_dir) .. '" --project-dir "' .. path.translate(root_dir) .. '" --namespace "' .. namespace .. '" --src-dir "' .. path.translate(src_dir) .. '"'
            print("  " .. cmd)
            os.run(cmd)
            print("=== Code Generation Complete ===\n")
        end, {dependfile = path.join(build_dir, "generated", ".codegen_stamp"),
              files = table.join(
                  os.files(path.join(src_dir, "**.xaml")),
                  os.files(path.join(src_dir, "**.xaml.h")),
                  os.files(path.join(src_dir, "**.idl"))
              )})
    end)

-- ── Rule: WinUI3 Post-Build ────────────────────────────────────
-- Handles copying Bootstrap DLL and .pri resource file
rule("winui3.postbuild")
    after_build(function (target)
        local outdir = target:targetdir()
        
        -- Copy Bootstrap DLL (now in Foundation sub-package for 1.8+)
        local ng = (os.getenv("USERPROFILE") or ""):gsub("\\", "/") .. "/.nuget/packages"
        local dll_src = (ng .. "/microsoft.windowsappsdk.foundation/1.8.260415000/runtimes/win-x64/native/Microsoft.WindowsAppRuntime.Bootstrap.dll"):gsub("/", "\\")
        local dll_dst = path.join(outdir, "Microsoft.WindowsAppRuntime.Bootstrap.dll")
        os.cp(dll_src, dll_dst)
        
        -- Copy .xbf files from generated/ to output root
        -- Note: xmake's os.files() does not support ** glob; XamlCompiler outputs all .xbf to root
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
            -- Also copy as <exename>.pri
            local named_dst = path.join(outdir, path.basename(target:targetfile()):gsub("%.exe$", ".pri"))
            os.cp(pri_src, named_dst)
        end
        
        print("[Post-build] Output: " .. path.translate(target:targetfile()))
    end)

-- ── Rule: WinUI3 Run ───────────────────────────────────────────
-- Runs the compiled WinUI3 application from its output directory
rule("winui3.run")
    on_run(function (target)
        os.runv(path.translate(target:targetfile()), {}, {curdir = target:targetdir()})
    end)
