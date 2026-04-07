add_rules("mode.debug", "mode.release")
set_languages("cxx20")
set_allowedplats("windows")
set_allowedarchs("x64")

local PACKAGE_VERSIONS = {
    ["Microsoft.Windows.CppWinRT"] = "2.0.250303.1",
    ["Microsoft.Windows.ImplementationLibrary"] = "1.0.260126.7",
    ["Microsoft.Windows.SDK.BuildTools"] = "10.0.28000.1721",
    ["Microsoft.WindowsAppSDK"] = "1.7.260224002",
    ["Microsoft.Web.WebView2"] = "1.0.3856.49"
}

local function _fail(message)
    if os.raise then
        os.raise(message)
    end
    local bad = nil
    return bad.value
end

local function _split_version(version)
    local values = {}
    for n in version:gmatch("%d+") do
        table.insert(values, tonumber(n))
    end
    return values
end

local function _compare_version(left, right)
    local lparts = _split_version(left)
    local rparts = _split_version(right)
    local total = math.max(#lparts, #rparts)
    for i = 1, total do
        local lv = lparts[i] or 0
        local rv = rparts[i] or 0
        if lv < rv then
            return -1
        end
        if lv > rv then
            return 1
        end
    end
    return 0
end

local function _latest_dir(pattern, prefix)
    local dirs = os.dirs(pattern)
    if #dirs == 0 then
        return nil
    end
    table.sort(dirs, function(left, right)
        local lname = path.filename(left)
        local rname = path.filename(right)
        if prefix then
            lname = lname:gsub("^" .. prefix, "")
            rname = rname:gsub("^" .. prefix, "")
        end
        return _compare_version(lname, rname) > 0
    end)
    return dirs[1]
end

local function _split_env_paths(paths)
    local result = {}
    if not paths then
        return result
    end
    for item in (paths .. ";"):gmatch("([^;]*);") do
        if item ~= "" then
            table.insert(result, path.translate(item))
        end
    end
    return result
end

local function _find_nuget_from_env()
    local env_candidates = {
        os.getenv("NUGET_EXE"),
        os.getenv("NUGET"),
        os.getenv("NuGetExePath")
    }

    for _, candidate in ipairs(env_candidates) do
        if candidate and candidate ~= "" then
            local translated = path.translate(candidate)
            if os.isfile(translated) then
                return translated
            end

            local exe = path.join(translated, "nuget.exe")
            if os.isfile(exe) then
                return exe
            end
        end
    end

    local path_env = os.getenv("PATH") or os.getenv("Path")
    for _, folder in ipairs(_split_env_paths(path_env)) do
        local exe = path.join(folder, "nuget.exe")
        if os.isfile(exe) then
            return exe
        end
    end
    return nil
end

local function _missing_packages(packages_dir)
    local missing = {}
    for id, version in pairs(PACKAGE_VERSIONS) do
        if not os.isdir(path.join(packages_dir, id .. "." .. version)) then
            table.insert(missing, id)
        end
    end
    table.sort(missing)
    return missing
end

local function _package_path(packages_dir, package_id)
    local version = PACKAGE_VERSIONS[package_id]
    if not version then
        _fail("missing package version for " .. package_id)
    end
    return path.join(packages_dir, package_id .. "." .. version)
end

local function _sdk_root()
    local from_env = os.getenv("WindowsSdkDir")
    if from_env and os.isdir(from_env) then
        return path.translate(from_env)
    end

    local root = "C:/Program Files (x86)/Windows Kits/10"
    if os.isdir(root) then
        return root
    end
    _fail("Windows 10 SDK root not found")
end

local function _platform_winmds(sdk_root, sdk_version)
    return os.files(path.join(sdk_root, "References", sdk_version, "*", "*", "*.winmd"))
end

local function _pick_first_existing_or_default(paths)
    for _, p in ipairs(paths) do
        if os.isfile(p) then
            return p
        end
    end
    return paths[1]
end

local function _appsdk_winmd_path(appsdk_lib, uap_dirs, name)
    local candidates = {}
    for _, dir_name in ipairs(uap_dirs) do
        table.insert(candidates, path.join(appsdk_lib, dir_name, name .. ".winmd"))
    end
    return _pick_first_existing_or_default(candidates)
end

local function _append_args(args, flag, values)
    for _, p in ipairs(values or {}) do
        table.insert(args, flag)
        table.insert(args, p)
    end
end

local function _make_cppwinrt_args(out_dir, inputs, refs)
    local args = {"-out", out_dir, "-optimize", "-prefix", "-overwrite"}
    _append_args(args, "-in", inputs)
    _append_args(args, "-ref", refs)
    return args
end

local function _target_context()
    local project_dir = os.projectdir()
    local packages_config = path.join(project_dir, "packages.config")
    local packages_dir = path.join(project_dir, ".xmake", "nuget")
    local missing = _missing_packages(packages_dir)
    local nuget = _find_nuget_from_env()
    if #missing > 0 and not nuget then
        _fail("nuget.exe not found from environment variables or PATH")
    end

    local sdk_root = _sdk_root()
    local include_dir = _latest_dir(path.join(sdk_root, "Include", "10.*"))
    if not include_dir then
        _fail("failed to find Windows SDK Include directory")
    end

    local sdk_ref_dir = _latest_dir(path.join(sdk_root, "References", "10.*"))
    if not sdk_ref_dir then
        _fail("failed to find Windows SDK References directory")
    end
    local sdk_version = path.filename(sdk_ref_dir)

    local platform_refs = _platform_winmds(sdk_root, sdk_version)
    local appsdk_path = _package_path(packages_dir, "Microsoft.WindowsAppSDK")
    local appsdk_lib = path.join(appsdk_path, "lib")

    local ver_uap_dirs = {"uap10.0.17763", "uap10.0.18362", "uap10.0.19041", "uap10.0"}
    local base_uap_dirs = {"uap10.0"}
    local appsdk_winmds = {
        _appsdk_winmd_path(appsdk_lib, ver_uap_dirs, "Microsoft.Foundation"),
        _appsdk_winmd_path(appsdk_lib, ver_uap_dirs, "Microsoft.Graphics"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.UI.Text"),
        _appsdk_winmd_path(appsdk_lib, ver_uap_dirs, "Microsoft.UI"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.UI.Xaml"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.ApplicationModel.DynamicDependency"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.ApplicationModel.Resources"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.ApplicationModel.WindowsAppRuntime"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.AppLifecycle"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.AppNotifications.Builder"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.AppNotifications"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.Management.Deployment"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.PushNotifications"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.Security.AccessControl"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.System.Power"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.System"),
        _appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.Widgets")
    }

    local webview2_winmd = path.join(_package_path(packages_dir, "Microsoft.Web.WebView2"), "lib", "Microsoft.Web.WebView2.Core.winmd")

    return {
        packages_config = packages_config,
        packages_dir = packages_dir,
        nuget = nuget,
        include_dir = include_dir,
        generated_dir = path.join(project_dir, ".xmake", "generated", "winrt"),
        cppwinrt = path.join(_package_path(packages_dir, "Microsoft.Windows.CppWinRT"), "bin", "cppwinrt.exe"),
        implementation_include = path.join(_package_path(packages_dir, "Microsoft.Windows.ImplementationLibrary"), "include"),
        appsdk_include = path.join(appsdk_path, "include"),
        appsdk_libdir = path.join(appsdk_path, "lib", "win10-x64"),
        bootstrap_dll = path.join(appsdk_path, "runtimes", "win-x64", "native", "Microsoft.WindowsAppRuntime.Bootstrap.dll"),
        platform_winmds = platform_refs,
        webview2_winmd = webview2_winmd,
        appsdk_winmds = appsdk_winmds
    }
end

target("test")
    set_kind("binary")
    add_files("src/main.cpp")
    add_defines("UNICODE", "_UNICODE")
    add_cxflags("/EHsc", "/Zc:__cplusplus", {force = true})
    add_ldflags("/subsystem:windows", {force = true})
    add_syslinks("ole32", "runtimeobject", "windowsapp", "user32")

    on_load(function(target)
        if not is_plat("windows") then
            _fail("this demo only supports Windows")
        end

        local ctx = _target_context()
        target:data_set("winui3_ctx", ctx)
        local system_include_dirs = {
            ctx.generated_dir,
            path.join(ctx.include_dir, "cppwinrt"),
            ctx.implementation_include,
            ctx.appsdk_include
        }
        for _, dir in ipairs(system_include_dirs) do
            target:add("sysincludedirs", dir)
        end
        target:add("linkdirs", ctx.appsdk_libdir)
        target:add("links", "Microsoft.WindowsAppRuntime.Bootstrap")
    end)

    before_buildcmd(function(target, batchcmds, opt)
        local ctx = target:data("winui3_ctx")
        if not ctx then
            _fail("missing WinUI3 context")
        end

        local missing = _missing_packages(ctx.packages_dir)
        if #missing > 0 then
            if not ctx.nuget then
                _fail("nuget.exe not found from environment variables or PATH")
            end
            batchcmds:show_progress(opt.progress, "${color.build.object}restoring.nuget %s", target:name())
            batchcmds:mkdir(ctx.packages_dir)
            batchcmds:vrunv(ctx.nuget, {
                "install",
                ctx.packages_config,
                "-OutputDirectory",
                ctx.packages_dir,
                "-NonInteractive"
            })
        end

        local args_platform = _make_cppwinrt_args(ctx.generated_dir, ctx.platform_winmds)
        local args_webview2 = _make_cppwinrt_args(ctx.generated_dir, {ctx.webview2_winmd}, ctx.platform_winmds)

        local appsdk_refs = {}
        for _, p in ipairs(ctx.platform_winmds) do
            table.insert(appsdk_refs, p)
        end
        table.insert(appsdk_refs, ctx.webview2_winmd)
        local args_appsdk = _make_cppwinrt_args(ctx.generated_dir, ctx.appsdk_winmds, appsdk_refs)

        batchcmds:show_progress(opt.progress, "${color.build.object}generating.cppwinrt.platform %s", target:name())
        batchcmds:mkdir(ctx.generated_dir)
        batchcmds:vrunv(ctx.cppwinrt, args_platform)

        batchcmds:show_progress(opt.progress, "${color.build.object}generating.cppwinrt.webview2 %s", target:name())
        batchcmds:vrunv(ctx.cppwinrt, args_webview2)

        batchcmds:show_progress(opt.progress, "${color.build.object}generating.cppwinrt.appsdk %s", target:name())
        batchcmds:vrunv(ctx.cppwinrt, args_appsdk)
    end)

    after_buildcmd(function(target, batchcmds)
        local ctx = target:data("winui3_ctx")
        if ctx then
            batchcmds:cp(ctx.bootstrap_dll, target:targetdir())
        end
    end)