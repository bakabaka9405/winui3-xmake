-- xmake rule for WinUI3 C++/WinRT build pipeline (dual-entry: pre-XAML + XAML/PRI)
--
-- This single rule wraps the entire WinUI3 application lifecycle:
--   1. Target configuration  (kind, filename, output dir, policy)
--   2. Compiler flags        (C++20, WinRT, Unicode)
--   3. NuGet include paths   (auto-discovered from packages.config)
--   4. Link libraries        (Bootstrap, WindowsApp)
--   5. Code generation       (two-phase via Python: pre-XAML entry + XAML/PRI entry, sharing build_winui3_common.py)
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

            --  ── 环境完整性检查 ─────────────────────────────────
            --  验证 packages.config 中所有包的本地目录是否真实存在。
            --  若缺失则自动执行 nuget restore 并重新验证。
            local env_ok, missing = nuget_cfg.check_env()
            if not env_ok then
                cprint("${color.warning}NuGet packages missing from global cache: %s",
                       table.concat(missing, ", "))
                cprint("${color.warning}Running 'nuget restore' to install missing packages...")
                os.run("nuget restore -PackagesDirectory \"%s\"",
                        os.getenv("USERPROFILE") .. "\\.nuget\\packages")
                --  清空 nuget_config 模块缓存，以强制重新解析
                nuget_cfg.reset_cache()

                --  恢复后再次验证
                local env_ok2, missing2 = nuget_cfg.check_env()
                if not env_ok2 then
                    raise("NuGet packages still missing after 'nuget restore':\n  "
                          .. table.concat(missing2, "\n  "))
                end
                cprint("${color.success}All NuGet packages verified.")
            end

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

        -- 全权在 on_load 中生成 XamlMetaDataProvider.idl 和 .cpp，
        -- Python 管线不再负责生成，仅负责将已存在的 .idl 纳入 MIDL 编译。
        local function write_file_if_changed(filepath, content)
            if os.isfile(filepath) then
                local existing = io.readfile(filepath)
                if existing == content then
                    return false
                end
            end
            io.writefile(filepath, content)
            return true
        end
        do
            local namespace = target:values("winui3.namespace") or "xmake_demo"
            local gen_dir = path.join(target:targetdir(), "generated")
            local xmp_idl = path.join(gen_dir, "XamlMetaDataProvider.idl")
            local xmp_file = path.join(gen_dir, "XamlMetaDataProvider.cpp")
            if not os.isdir(gen_dir) then
                os.mkdir(gen_dir)
            end
            -- 生成 .idl（使用正确的命名空间）
            local idl_content = string.format(
                "namespace %s\n{\n    runtimeclass XamlMetaDataProvider : [default] Microsoft.UI.Xaml.Markup.IXamlMetadataProvider\n    {\n        XamlMetaDataProvider();\n    }\n}\n",
                namespace
            )
            write_file_if_changed(xmp_idl, idl_content)
            -- 生成 .cpp 存根（包含 cppwinrt 生成的实现文件）
            write_file_if_changed(xmp_file, '#include "pch.h"\n#include "XamlMetaDataProvider.h"\n#include "XamlMetaDataProvider.g.cpp"\n')
            -- 注册 .cpp 为编译源文件
            target:add("files", xmp_file)
        end

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

    --  before_build: two-entry code generation via pre-XAML and XAML/PRI Python scripts
    before_build(function (target)
        import("core.project.depend")

        local namespace = target:values("winui3.namespace") or "xmake_demo"
        local src_dir   = target:values("winui3.src_dir") or path.join(os.projectdir(), "src")
        local root_dir  = target:values("winui3.root_dir") or os.projectdir()
        local build_dir = target:targetdir()
        local build_dir_root = path.directory(build_dir)  -- e.g. build/windows/release/x64
        local shared_projection_dir = path.join(build_dir_root, "shared", "generated")

        -- Helper: build Python script arguments for pre-XAML entry (phases 1-4)
        local function py_args_pre()
            local script = path.join(root_dir, "scripts/build_winui3_pre_xaml.py")
            local args   = {
                path.translate(script),
                "--build-dir", path.translate(build_dir),
                "--project-dir", path.translate(root_dir),
                "--namespace", namespace,
                "--src-dir", path.translate(src_dir),
                "--shared-projection-dir", path.translate(shared_projection_dir),
            }
            return args
        end

        -- Helper: build Python script arguments for XAML/PRI entry (phases 6-8)
        local function py_args_xaml()
            local script = path.join(root_dir, "scripts/build_winui3_xaml_pri.py")
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

            return args
        end

        -- Stamp 1 -- Pre-XAML generation (shared projection, MIDL, mdmerge, cppwinrt)
        -- Runs phases 1-4 via scripts/build_winui3_pre_xaml.py.
        -- Includes generated XamlMetaDataProvider files.
        local idl_stamp_file = path.join(build_dir, "generated", ".idl_stamp")

        depend.on_changed(function ()
            local py   = "python"
            local args = py_args_pre()
            os.runv(py, args)
        end, {
            dependfile = idl_stamp_file,
            files = table.join(
                os.files(path.join(src_dir, "**.idl")),
                os.files(path.join(build_dir, "generated", "XamlMetaDataProvider.idl")),
                os.files(path.join(root_dir, "scripts", "winmd", "**.py")),
                {
                    path.join(build_dir, "generated", "XamlMetaDataProvider.cpp"),
                    path.join(root_dir, "scripts", "build_winui3_common.py"),
                    path.join(root_dir, "scripts", "build_winui3_pre_xaml.py"),
                    path.join(root_dir, "scripts", "nuget_config.py"),
                    path.join(root_dir, "scripts", "nuget_config.lua"),
                    path.join(root_dir, "scripts", "plat_info.py"),
                    path.join(root_dir, "rules", "winui3.lua"),
                    path.join(root_dir, "packages.config"),
                }
            ),
        })

        -- Stamp 2 -- XAML compilation and resource indexing
        -- Runs phases 6-8 via scripts/build_winui3_xaml_pri.py.
        -- DEPENDS on Stamp 1's dependfile to ensure IDL changes cascade to XAML.
        depend.on_changed(function ()
            local py   = "python"
            local args = py_args_xaml()
            os.runv(py, args)
        end, {
            dependfile = path.join(build_dir, "generated", ".xaml_stamp"),
            files = table.join(
                os.files(path.join(src_dir, "**.xaml")),
                os.files(path.join(src_dir, "**.xaml.h")),
                {
                    path.join(root_dir, "scripts", "build_winui3_common.py"),
                    path.join(root_dir, "scripts", "build_winui3_xaml_pri.py"),
                    path.join(root_dir, "rules", "winui3.lua"),
                    idl_stamp_file,  -- cascade IDL changes to XAML
                }
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
