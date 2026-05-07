-- xmake rule for WinUI3 C++/WinRT build pipeline
--
-- This single rule wraps the entire WinUI3 application lifecycle:
--   1. Target configuration  (kind, filename, output dir, policy)
--   2. Compiler flags        (C++20, WinRT, Unicode)
--   3. NuGet include paths   (auto-discovered from packages.config)
--   4. Link libraries        (Bootstrap, WindowsApp)
--   5. Code generation       (XAML + IDL → WinMD → projection via Python)
--   6. Post-build            (copy Bootstrap DLL and resources.pri to output)
--   7. Run                   (execute from output directory)

-- ── 构建规则 ─────────────────────────────────────────────────

-- 模块级缓存：首次解析后缓存 packages.config 的包路径映射与声明顺序
local _cached_nuget_paths = nil
local _cached_nuget_package_ids = nil

rule("winui3.app")
    --  on_load: configure target identity, search paths, compiler & linker flags 
    on_load(function (target)
        --  Parse NuGet package paths from packages.config (cached)
        if not _cached_nuget_paths then
            local nuget_cfg = import("scripts.nuget_config", {rootdir = os.projectdir()})
            _cached_nuget_paths, _cached_nuget_package_ids = nuget_cfg.all_packages()
        end
        local paths = _cached_nuget_paths
        local package_ids = _cached_nuget_package_ids or {}

        local NUGET_FOUNDATION = paths["Microsoft.WindowsAppSDK.Foundation"]

        --  Target identity 
        target:set("kind", "binary")
        target:set("filename", target:name() .. ".exe")
        target:set("targetdir", path.join("$(builddir)", "$(host)", "$(mode)", "$(arch)", target:name()))
        target:set("rundir", target:targetdir())
        target:set("policy", "build.across_targets_in_parallel", false)

        if not target:values("winui3.root_dir") then
            target:set("values", "winui3.root_dir", os.projectdir())
        end

        --  Generated output include directories 
        target:add("includedirs", path.join(target:targetdir(), "generated"))
        target:add("includedirs", path.join(target:targetdir(), "generated", "sources"))
        target:add("includedirs", path.join(path.directory(target:targetdir()), "shared", "generated"))

        --  NuGet include directories: add every package include/ folder that exists.
        for _, pid in ipairs(package_ids) do
            local include_dir = path.join(paths[pid], "include")
            if os.isdir(include_dir) then
                target:add("includedirs", include_dir)
            end
        end

        local src_dir = target:values("winui3.src_dir")
        if src_dir then
            target:add("includedirs", src_dir)
        end

        local namespace = target:values("winui3.namespace") or "xmake_demo"

        --  Compiler flags 
        target:add("cxflags", "/EHsc", "/bigobj", "/await:strict", "/utf-8",
                    "/DNOMINMAX", "/DWIN32_LEAN_AND_MEAN", "/DUNICODE", "/D_UNICODE",
                    "/DDISABLE_XAML_GENERATED_MAIN", "/DWINUI3_APP_NAMESPACE=" .. namespace)

        --  Link libraries 
        target:add("links", path.translate(NUGET_FOUNDATION .. "/lib/native/x64/Microsoft.WindowsAppRuntime.Bootstrap.lib"))
        target:add("links", "windowsapp")
        target:add("links", "user32")
        target:add("ldflags", "/SUBSYSTEM:WINDOWS")
    end)

    --  before_build: code generation (Python orchestrator) 
    before_build(function (target)
        import("core.project.depend")

        local namespace = target:values("winui3.namespace") or "xmake_demo"
        local src_dir   = target:values("winui3.src_dir") or path.join(os.projectdir(), "src")
        local root_dir  = target:values("winui3.root_dir") or os.projectdir()
        local build_dir = target:targetdir()
        local shared_projection_dir = path.join(path.directory(build_dir), "shared", "generated")

        depend.on_changed(function ()
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

            local xaml_compiler_path = target:values("winui3.xaml_compiler_path")
            if xaml_compiler_path then
                table.insert(args, "--xaml-compiler-path")
                table.insert(args, path.translate(xaml_compiler_path))
            end

            os.runv(py, args)
        end, {
            dependfile = path.join(build_dir, "generated", ".codegen_stamp"),
            files = table.join(
                os.files(path.join(src_dir, "**.xaml")),
                os.files(path.join(src_dir, "**.xaml.h")),
                os.files(path.join(src_dir, "**.idl")),
                os.files(path.join(root_dir, "scripts", "winmd", "**.py")),
                { path.join(root_dir, "scripts", "build_winui3.py"), path.join(root_dir, "scripts", "plat_info.py"), path.join(root_dir, "scripts", "nuget_config.py"), path.join(root_dir, "scripts", "nuget_config.lua"), path.join(root_dir, "rules", "winui3.lua"), path.join(root_dir, "packages.config") }
            ),
        })
    end)

    --  after_build: copy runtime files to output
    --  Depends on _cached_nuget_paths populated by on_load (runs first in target lifecycle).
    after_build(function (target)
        local nuget_cfg = import("scripts.nuget_config", {rootdir = os.projectdir()})
        local foundation = nuget_cfg.package_path("Microsoft.WindowsAppSDK.Foundation")
        if not foundation then
            raise("Microsoft.WindowsAppSDK.Foundation NuGet package not found in packages.config")
        end

        local outdir = target:targetdir()
        local dll_src = (foundation .. "/runtimes/win-x64/native/Microsoft.WindowsAppRuntime.Bootstrap.dll"):gsub("/", "\\")
        local dll_dst = path.join(outdir, "Microsoft.WindowsAppRuntime.Bootstrap.dll")
        os.cp(dll_src, dll_dst)

        os.cp(path.join(outdir, "generated", "resources.pri"), path.join(outdir, "resources.pri"))
    end)
