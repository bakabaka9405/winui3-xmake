add_rules("mode.debug", "mode.release")
set_languages("cxx20")
set_allowedplats("windows")
set_allowedarchs("x64")

local ROOT_NAMESPACE = "xmake_demo"

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

local function _filter_core_platform_winmds(platform_refs)
    local core = {}
    for _, p in ipairs(platform_refs or {}) do
        local name = path.filename(p)
        if name == "Windows.Foundation.FoundationContract.winmd" or name == "Windows.Foundation.UniversalApiContract.winmd" then
            table.insert(core, p)
        end
    end
    if #core == 0 and #platform_refs > 0 then
        table.insert(core, platform_refs[1])
    end
    return core
end

local function _append_unique(list, value)
    for _, item in ipairs(list) do
        if item == value then
            return
        end
    end
    table.insert(list, value)
end

local function _json_escape(value)
    local text = value or ""
    text = text:gsub("\\", "\\\\")
    text = text:gsub("\"", "\\\"")
    text = text:gsub("\r", "\\r")
    text = text:gsub("\n", "\\n")
    return text
end

local function _json_string(value)
    return "\"" .. _json_escape(value) .. "\""
end

local function _json_array(items)
    return "[" .. table.concat(items, ",") .. "]"
end

local function _make_msbuild_item(path_value, depends_value)
    local fields = {
        "\"ItemSpec\":" .. _json_string(path_value),
        "\"FullPath\":" .. _json_string(path_value)
    }
    if depends_value then
        table.insert(fields, "\"DependentUpon\":" .. _json_string(depends_value))
    end
    return "{" .. table.concat(fields, ",") .. "}"
end

local function _ps_single_quote(value)
    return (value or ""):gsub("'", "''")
end

local function _batch_write_file(batchcmds, file_path, content)
    local script = "$text=@'\n" .. content .. "\n'@; Set-Content -LiteralPath '" .. _ps_single_quote(path.translate(file_path)) .. "' -Value $text -Encoding UTF8"
    batchcmds:vrunv("powershell", {
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-Command",
        script
    })
end

local function _xaml_json_input(ctx, is_pass1)
    local ref_assemblies = {}
    for _, ref_path in ipairs(ctx.winmd_refs) do
        table.insert(ref_assemblies, _make_msbuild_item(ref_path))
    end

    local xaml_pages = {}
    for _, page_path in ipairs(ctx.xaml_pages) do
        table.insert(xaml_pages, _make_msbuild_item(page_path))
    end

    local xaml_apps = {}
    for _, app_path in ipairs(ctx.xaml_apps) do
        table.insert(xaml_apps, _make_msbuild_item(app_path))
    end

    local cl_include_files = {}
    for _, xaml_path in ipairs(ctx.xaml_apps) do
        table.insert(cl_include_files, _make_msbuild_item(xaml_path .. ".h", xaml_path))
    end
    for _, xaml_path in ipairs(ctx.xaml_pages) do
        table.insert(cl_include_files, _make_msbuild_item(xaml_path .. ".h", xaml_path))
    end

    local fields = {
        "\"SavedStateFile\":" .. _json_string(ctx.xaml_saved_state),
        "\"IsPass1\":" .. (is_pass1 and "true" or "false"),
        "\"Language\":" .. _json_string("CppWinRT"),
        "\"ProjectPath\":" .. _json_string(ctx.project_script),
        "\"LanguageSourceExtension\":" .. _json_string(".cpp"),
        "\"OutputPath\":" .. _json_string(ctx.xaml_generated_dir),
        "\"RootNamespace\":" .. _json_string(ctx.root_namespace),
        "\"PrecompiledHeaderFile\":" .. _json_string("pch.h"),
        "\"FeatureControlFlags\":" .. _json_string("EnableXBindDiagnostics;EnableDefaultValidationContextGeneration;EnableWin32Codegen"),
        "\"ReferenceAssemblies\":" .. _json_array(ref_assemblies),
        "\"ReferenceAssemblyPaths\":[]",
        "\"TargetPlatformMinVersion\":" .. _json_string(ctx.sdk_version),
        "\"XamlPages\":" .. _json_array(xaml_pages),
        "\"XamlApplications\":" .. _json_array(xaml_apps),
        "\"ClIncludeFiles\":" .. _json_array(cl_include_files)
    }

    if not is_pass1 then
        table.insert(fields, "\"LocalAssembly\":" .. _json_array({_make_msbuild_item(ctx.merged_winmd)}))
        table.insert(fields, "\"GenXbfPath\":" .. _json_string(ctx.appsdk_tools_dir))
    end

    return "{" .. table.concat(fields, ",") .. "}"
end

local function _write_xaml_input_json(batchcmds, ctx)
    _batch_write_file(batchcmds, ctx.xaml_pass1_in_json, _xaml_json_input(ctx, true))
    _batch_write_file(batchcmds, ctx.xaml_pass2_in_json, _xaml_json_input(ctx, false))
end

local function _write_makepri_inputs(batchcmds, ctx)
    local resfiles = table.concat(ctx.xaml_xbf_files, "\n")
    _batch_write_file(batchcmds, ctx.makepri_resfiles, resfiles)

    local xml_lines = {
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>",
        "<resources targetOsVersion=\"10.0.0\" majorVersion=\"1\">",
        "<index root=\"" .. _json_escape(ctx.xaml_generated_dir) .. "\" startIndexAt=\"" .. _json_escape(ctx.makepri_resfiles) .. "\">",
        "<default>",
        "<qualifier name=\"Language\" value=\"en-US\" />",
        "<qualifier name=\"Contrast\" value=\"standard\" />",
        "<qualifier name=\"Scale\" value=\"200\" />",
        "<qualifier name=\"HomeRegion\" value=\"001\" />",
        "<qualifier name=\"TargetSize\" value=\"256\" />",
        "<qualifier name=\"LayoutDirection\" value=\"LTR\" />",
        "<qualifier name=\"DXFeatureLevel\" value=\"DX9\" />",
        "<qualifier name=\"Configuration\" value=\"\" />",
        "<qualifier name=\"AlternateForm\" value=\"\" />",
        "<qualifier name=\"Platform\" value=\"UAP\" />",
        "</default>",
        "<indexer-config type=\"RESFILES\" qualifierDelimiter=\".\" />",
        "<indexer-config type=\"EMBEDFILES\" />",
        "</index>",
        "</resources>"
    }
    _batch_write_file(batchcmds, ctx.makepri_config, table.concat(xml_lines, "\n"))
end

local function _find_foundation_metadata_dir(sdk_root, sdk_version)
    local refs_root = path.join(sdk_root, "References", sdk_version)
    local candidates = {
        path.join(refs_root, "Windows.Foundation.FoundationContract", "*"),
        path.join(refs_root, "windows.foundation.foundationcontract", "*")
    }

    for _, pattern in ipairs(candidates) do
        local dirs = os.dirs(pattern)
        if #dirs > 0 then
            table.sort(dirs, function(left, right)
                return _compare_version(path.filename(left), path.filename(right)) > 0
            end)
            return dirs[1]
        end
    end
    _fail("failed to find Windows.Foundation.FoundationContract metadata directory")
end

local function _sdk_buildtools_bindir(packages_dir)
    local sdk_tools = _package_path(packages_dir, "Microsoft.Windows.SDK.BuildTools")
    local version_dir = _latest_dir(path.join(sdk_tools, "bin", "*"))
    if not version_dir then
        _fail("failed to find SDK BuildTools bin directory")
    end
    local x64_dir = path.join(version_dir, "x64")
    if not os.isdir(x64_dir) then
        _fail("failed to find x64 SDK BuildTools bin directory")
    end
    return x64_dir
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
    local buildtools_bindir = _sdk_buildtools_bindir(packages_dir)
    local foundation_metadata_dir = _find_foundation_metadata_dir(sdk_root, sdk_version)

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

    local xmake_root = path.join(project_dir, ".xmake")
    local generated_root = path.join(xmake_root, "generated")
    local generated_winrt = path.join(generated_root, "winrt")
    local generated_xaml = path.join(generated_root, "xaml")
    local generated_winmd_unmerged = path.join(generated_root, "winmd_unmerged")
    local generated_winmd_merged = path.join(generated_root, "winmd_merged")

    local root_namespace = ROOT_NAMESPACE
    local merged_winmd = path.join(generated_winmd_merged, root_namespace .. ".winmd")
    local mainwindow_idl = path.join(project_dir, "src", "MainWindow.idl")
    local xamlmetadata_idl = path.join(project_dir, "src", "XamlMetaDataProvider.idl")
    local mainwindow_winmd = path.join(generated_winmd_unmerged, "MainWindow.winmd")
    local xamlmetadata_winmd = path.join(generated_winmd_unmerged, "XamlMetaDataProvider.winmd")
    local app_xaml = path.join(project_dir, "src", "App.xaml")
    local mainwindow_xaml = path.join(project_dir, "src", "MainWindow.xaml")

    local core_platform_refs = _filter_core_platform_winmds(platform_refs)

    local winmd_refs = {}
    for _, p in ipairs(core_platform_refs) do
        table.insert(winmd_refs, p)
    end
    for _, p in ipairs(appsdk_winmds) do
        table.insert(winmd_refs, p)
    end
    table.insert(winmd_refs, webview2_winmd)

    return {
        packages_config = packages_config,
        packages_dir = packages_dir,
        project_dir = project_dir,
        nuget = nuget,
        include_dir = include_dir,
        generated_dir = generated_winrt,
        cppwinrt = path.join(_package_path(packages_dir, "Microsoft.Windows.CppWinRT"), "bin", "cppwinrt.exe"),
        implementation_include = path.join(_package_path(packages_dir, "Microsoft.Windows.ImplementationLibrary"), "include"),
        appsdk_include = path.join(appsdk_path, "include"),
        appsdk_libdir = path.join(appsdk_path, "lib", "win10-x64"),
        appsdk_tools_dir = path.join(appsdk_path, "tools"),
        xaml_compiler = path.join(appsdk_path, "tools", "net472", "XamlCompiler.exe"),
        midl = path.join(buildtools_bindir, "midl.exe"),
        mdmerge = path.join(buildtools_bindir, "mdmerge.exe"),
        makepri = path.join(buildtools_bindir, "makepri.exe"),
        foundation_metadata_dir = foundation_metadata_dir,
        bootstrap_dll = path.join(appsdk_path, "runtimes", "win-x64", "native", "Microsoft.WindowsAppRuntime.Bootstrap.dll"),
        platform_winmds = platform_refs,
        webview2_winmd = webview2_winmd,
        appsdk_winmds = appsdk_winmds,
        root_namespace = root_namespace,
        project_script = path.join(project_dir, "xmake.lua"),
        source_dir = path.join(project_dir, "src"),
        sdk_version = sdk_version,
        winmd_refs = winmd_refs,
        winmd_unmerged_dir = generated_winmd_unmerged,
        winmd_merged_dir = generated_winmd_merged,
        mainwindow_idl = mainwindow_idl,
        xamlmetadata_idl = xamlmetadata_idl,
        mainwindow_winmd = mainwindow_winmd,
        xamlmetadata_winmd = xamlmetadata_winmd,
        merged_winmd = merged_winmd,
        xaml_generated_dir = generated_xaml,
        xaml_generated_src_dir = path.join(generated_xaml, "src"),
        xaml_apps = {app_xaml},
        xaml_pages = {mainwindow_xaml},
        xaml_saved_state = path.join(generated_xaml, "XamlCompilerState.xml"),
        xaml_pass1_in_json = path.join(xmake_root, "xaml.pass1.in.json"),
        xaml_pass1_out_json = path.join(xmake_root, "xaml.pass1.out.json"),
        xaml_pass2_in_json = path.join(xmake_root, "xaml.pass2.in.json"),
        xaml_pass2_out_json = path.join(xmake_root, "xaml.pass2.out.json"),
        xaml_xbf_files = {"src/App.xbf", "src/MainWindow.xbf"},
        makepri_config = path.join(xmake_root, "resources.pri.xml"),
        makepri_resfiles = path.join(xmake_root, "resources.pri.resfiles"),
        pri_file = path.join(generated_xaml, "resources.pri")
    }
end

target("test")
    set_kind("binary")
    add_files("src/*.cpp")
    add_defines("UNICODE", "_UNICODE", "DISABLE_XAML_GENERATED_MAIN")
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
            ctx.xaml_generated_dir,
            ctx.xaml_generated_src_dir,
            ctx.generated_dir,
            path.join(ctx.include_dir, "cppwinrt"),
            ctx.implementation_include,
            ctx.appsdk_include,
            ctx.project_dir,
            ctx.source_dir
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

        batchcmds:show_progress(opt.progress, "${color.build.object}generating.winmd.mainwindow %s", target:name())
        os.mkdir(ctx.winmd_unmerged_dir)
        local cpp_cmd = target:tool("cxx") or "cl.exe"
        local cpp_dir = path.directory(cpp_cmd)
        local midl_envs = {PATH = cpp_dir .. ";" .. (os.getenv("PATH") or "")}
        local midl_include_dirs = {
            path.join(ctx.include_dir, "winrt"),
            path.join(ctx.include_dir, "shared"),
            path.join(ctx.include_dir, "um"),
            path.join(ctx.include_dir, "ucrt")
        }
        local mainwindow_midl_args = {
            ctx.mainwindow_idl,
            "/nologo",
            "/winrt",
            "/winmd",
            ctx.mainwindow_winmd,
            "/nomidl",
            "/h",
            "nul",
            "/metadata_dir",
            ctx.foundation_metadata_dir
        }
        _append_args(mainwindow_midl_args, "/I", midl_include_dirs)
        _append_args(mainwindow_midl_args, "/reference", ctx.winmd_refs)
        batchcmds:vrunv(ctx.midl, mainwindow_midl_args, {envs = midl_envs})

        batchcmds:show_progress(opt.progress, "${color.build.object}generating.winmd.xamlmetadata %s", target:name())
        local xamlmeta_midl_args = {
            ctx.xamlmetadata_idl,
            "/nologo",
            "/winrt",
            "/winmd",
            ctx.xamlmetadata_winmd,
            "/nomidl",
            "/h",
            "nul",
            "/metadata_dir",
            ctx.foundation_metadata_dir
        }
        _append_args(xamlmeta_midl_args, "/I", midl_include_dirs)
        _append_args(xamlmeta_midl_args, "/reference", ctx.winmd_refs)
        batchcmds:vrunv(ctx.midl, xamlmeta_midl_args, {envs = midl_envs})

        batchcmds:show_progress(opt.progress, "${color.build.object}merging.winmd %s", target:name())
        os.mkdir(ctx.winmd_merged_dir)
        local mdmerge_args = {"-o", ctx.winmd_merged_dir}
        local metadata_dirs = {}
        for _, ref_path in ipairs(ctx.winmd_refs) do
            _append_unique(metadata_dirs, path.directory(ref_path))
        end
        _append_args(mdmerge_args, "-metadata_dir", metadata_dirs)
        table.insert(mdmerge_args, "-i")
        table.insert(mdmerge_args, ctx.mainwindow_winmd)
        table.insert(mdmerge_args, "-i")
        table.insert(mdmerge_args, ctx.xamlmetadata_winmd)
        table.insert(mdmerge_args, "-partial")
        table.insert(mdmerge_args, "-n:1")
        batchcmds:vrunv(ctx.mdmerge, mdmerge_args)

        batchcmds:show_progress(opt.progress, "${color.build.object}generating.cppwinrt.component %s", target:name())
        local args_component = {
            "-out",
            ctx.generated_dir,
            "-comp",
            ctx.generated_dir,
            "-name",
            ctx.root_namespace,
            "-optimize",
            "-prefix",
            "-overwrite",
            "-pch",
            "pch.h",
            "-in",
            ctx.merged_winmd
        }
        _append_args(args_component, "-ref", ctx.winmd_refs)
        batchcmds:vrunv(ctx.cppwinrt, args_component)

        batchcmds:show_progress(opt.progress, "${color.build.object}generating.xaml.inputs %s", target:name())
        _write_xaml_input_json(batchcmds, ctx)

        batchcmds:show_progress(opt.progress, "${color.build.object}compiling.xaml.pass1 %s", target:name())
        batchcmds:vrunv(ctx.xaml_compiler, {ctx.xaml_pass1_in_json, ctx.xaml_pass1_out_json})

        batchcmds:show_progress(opt.progress, "${color.build.object}compiling.xaml.pass2 %s", target:name())
        batchcmds:vrunv(ctx.xaml_compiler, {ctx.xaml_pass2_in_json, ctx.xaml_pass2_out_json})

        batchcmds:show_progress(opt.progress, "${color.build.object}generating.pri %s", target:name())
        _write_makepri_inputs(batchcmds, ctx)
        batchcmds:vrunv(ctx.makepri, {
            "new",
            "/cf",
            ctx.makepri_config,
            "/pr",
            os.projectdir(),
            "/o",
            "/of",
            ctx.pri_file
        })
    end)

    after_buildcmd(function(target, batchcmds)
        local ctx = target:data("winui3_ctx")
        if ctx then
            batchcmds:cp(ctx.bootstrap_dll, target:targetdir())
            batchcmds:cp(ctx.pri_file, path.join(target:targetdir(), "test.pri"))
        end
    end)