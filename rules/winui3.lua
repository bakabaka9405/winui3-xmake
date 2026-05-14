-- xmake rule for WinUI3 C++/WinRT build pipeline (stamp-based: shared-projection -> pre-XAML -> XAML/PRI)
--
-- Lifecycle phases:
--   1. on_load:    Structural identity (kind, filename, output dir, policy) + parameter validation
--   2. on_config:  NuGet environment check (delegated to "nuget-check" plugin)
--                  + NuGet-dependent includes, compiler flags, link libraries
--   3. on_prepare: Code generation via Python (pre-XAML entry + XAML/PRI entry,
--                  sharing build_winui3_common.py)
--   4. after_build: Copy Bootstrap DLL and resources.pri to output

-- ── 构建规则 ─────────────────────────────────────────────────

-- 模块级缓存：首次解析后缓存 packages.config 的包路径映射与声明顺序
local _cached_nuget_paths = nil
local _cached_nuget_package_ids = nil

-- 共享 WinRT 投影头的全局生成目录（所有目标、所有配置共用）
local function shared_projection_dir()
    return path.join(os.projectdir(), "build", ".gens", "shared", "generated")
end

rule("winui3.app")
    --  on_load: configure target structural identity (kind, filename, output dirs).
    --  NuGet environment check and all NuGet-dependent configuration deferred to on_config.
    on_load(function (target)
        --  Target identity —— 必须在加载阶段确定的属性
        target:set("kind", "binary")
        target:set("filename", target:name() .. ".exe")
        target:set("targetdir", path.join(target:targetdir(), target:name()))
        target:set("rundir", target:targetdir())
        target:set("policy", "build.across_targets_in_parallel", false)

        --  参数校验
        local namespace = target:extraconf("rules", "winui3.app", "namespace")
        if namespace == nil or type(namespace) ~= "string" then
            raise("winui3.app requires \"namespace\" parameter.\nUsage: add_rules(\"winui3.app\", {namespace = \"YourNamespace\", src_dir = path.join(os.scriptdir(), \"src\")})")
        end

        local src_dir = target:extraconf("rules", "winui3.app", "src_dir")
        if src_dir == nil or type(src_dir) ~= "string" then
            raise("winui3.app requires \"src_dir\" parameter.\nUsage: add_rules(\"winui3.app\", {namespace = \"YourNamespace\", src_dir = path.join(os.scriptdir(), \"src\")})")
        end
        if not os.isdir(src_dir) then
            raise("winui3.app: \"src_dir\" path does not exist: " .. src_dir .. "\nUsage: add_rules(\"winui3.app\", {namespace = \"YourNamespace\", src_dir = path.join(os.scriptdir(), \"src\")})")
        end
    end)

    --  on_config: NuGet environment verification + all NuGet-dependent configuration.
    --  延迟至配置阶段执行：在 xmake f（或隐式配置）之后、构建之前运行。
    --  通过 task.run("nuget-check") 将环境检查委托给独立的 plugin task，
    --  确保逻辑可从 CLI 和构建流程中同时复用。
    on_config(function (target)
        --  Stage 1: NuGet environment verification（guarded — 所有目标仅执行一次）
        if not _cached_nuget_paths then
            local task = import("core.project.task")
            task.run("nuget-check")
            local nuget_cfg = import("scripts.nuget_config", {rootdir = os.projectdir()})
            _cached_nuget_paths, _cached_nuget_package_ids = nuget_cfg.all_packages()
        end
        local paths = _cached_nuget_paths
        local package_ids = _cached_nuget_package_ids or {}

        local namespace = target:extraconf("rules", "winui3.app", "namespace")
        local src_dir   = target:extraconf("rules", "winui3.app", "src_dir")
        local autogen_dir = target:autogendir({root = true})

        --  Stage 2: Include directories —— 生成输出、共享投影、NuGet 包、源码目录
        target:add("includedirs", path.join(autogen_dir, "generated"))
        target:add("includedirs", path.join(autogen_dir, "generated", "sources"))
        target:add("includedirs", shared_projection_dir())

        --  NuGet include directories: add every package include/ folder that exists.
        for _, pid in ipairs(package_ids) do
            local include_dir = path.join(paths[pid], "include")
            if os.isdir(include_dir) then
                target:add("includedirs", include_dir)
            end
        end

        target:add("includedirs", src_dir)

        --  Stage 3: Compiler flags
        target:add("cxflags", "/EHsc", "/bigobj", "/await:strict", "/utf-8",
                    "/DNOMINMAX", "/DWIN32_LEAN_AND_MEAN", "/DUNICODE", "/D_UNICODE",
                    "/DDISABLE_XAML_GENERATED_MAIN", "/DWINUI3_APP_NAMESPACE=" .. namespace)

        --  Stage 4: Link libraries
        local NUGET_FOUNDATION = paths["Microsoft.WindowsAppSDK.Foundation"]
        target:add("links", path.translate(NUGET_FOUNDATION .. "/lib/native/x64/Microsoft.WindowsAppRuntime.Bootstrap.lib"))
        target:add("links", "windowsapp")
        target:add("links", "user32")
        target:add("ldflags", "/SUBSYSTEM:WINDOWS")
    end)

    --  on_prepare: two-entry code generation via pre-XAML and XAML/PRI Python scripts
    on_prepare(function (target)
        -- NOTE: xmake gen invokes generation helpers directly without
        -- compile/link. Stamps (.idl_stamp, .xaml_stamp) in this on_prepare
        -- are ONLY updated by xmake build's depend.on_changed mechanism.
        import("core.project.depend")
        local task = import("core.project.task")
        task.run("gen-xmdp", {target = target:name()})

        local namespace = target:extraconf("rules", "winui3.app", "namespace")
        local src_dir   = target:extraconf("rules", "winui3.app", "src_dir")
        local root_dir  = os.projectdir()
        -- build_dir uses autogendir as root path for all generated intermediate files
        local build_dir = target:autogendir({root = true})
        local shared_gen = shared_projection_dir()

        -- Stamp 0 -- Shared C++/WinRT projection header generation (Phase 0)
        -- 增量保护由 Python 端的 projection_fingerprint() 负责；Lua 侧仅保留
        -- stamp 文件路径供 Stamp 1 的级联依赖使用。
        task.run("gen-winrt-shared")

        -- Stamp 1 -- Pre-XAML generation (MIDL, mdmerge, cppwinrt)
        -- Runs phases 2-4 via scripts/build_winui3_pre_xaml.py.
        -- Includes generated XamlMetaDataProvider files.
        local idl_stamp_file = path.join(build_dir, "generated", ".idl_stamp")

        depend.on_changed(function ()
            local py   = "python"
            local script = path.join(root_dir, "scripts/build_winui3_pre_xaml.py")
            local args = {
                path.translate(script),
                "--build-dir",             path.translate(build_dir),
                "--project-dir",           path.translate(root_dir),
                "--namespace",             namespace,
                "--src-dir",               path.translate(src_dir),
                "--shared-projection-dir", path.translate(shared_gen),
            }
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
                    path.join(shared_gen, ".shared_projection_stamp.json"),
                }
            ),
        })

        -- Stamp 2 -- XAML compilation and resource indexing
        -- Runs phases 6-8 via scripts/build_winui3_xaml_pri.py.
        -- DEPENDS on Stamp 1's dependfile to ensure IDL changes cascade to XAML.
        local xaml_compiler_path = get_config("winui3.xaml_compiler_path")
        depend.on_changed(function ()
            local py   = "python"
            local script = path.join(root_dir, "scripts/build_winui3_xaml_pri.py")
            local args = {
                path.translate(script),
                "--build-dir",             path.translate(build_dir),
                "--project-dir",           path.translate(root_dir),
                "--namespace",             namespace,
                "--src-dir",               path.translate(src_dir),
                "--shared-projection-dir", path.translate(shared_gen),
            }

            -- xaml_compiler_path 为可选参数；仅在提供时追加
            if xaml_compiler_path then
                table.insert(args, "--xaml-compiler-path")
                table.insert(args, path.translate(xaml_compiler_path))
            end
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

    --  after_build: copy runtime files to output.
    --  nuget_config.lua cache populated by on_config (runs before build); after_build
    --  re-imports nuget_config independently to resolve the Foundation package path.
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

        os.cp(path.join(target:autogendir({root = true}), "generated", "resources.pri"), path.join(outdir, "resources.pri"))
    end)
