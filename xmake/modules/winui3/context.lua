local ROOT_NAMESPACE = "xmake_demo"

local PACKAGE_VERSIONS = {
    ["Microsoft.Windows.CppWinRT"] = "2.0.250303.1",
    ["Microsoft.Windows.ImplementationLibrary"] = "1.0.260126.7",
    ["Microsoft.Windows.SDK.BuildTools"] = "10.0.28000.1721",
    ["Microsoft.WindowsAppSDK"] = "1.7.260224002",
    ["Microsoft.Web.WebView2"] = "1.0.3856.49"
}

local util = import("xmake.modules.winui3.utils", {rootdir = os.projectdir()})

function missing_packages(packages_dir)
    local missing = {}
    for id, version in pairs(PACKAGE_VERSIONS) do
        if not os.isdir(path.join(packages_dir, id .. "." .. version)) then
            table.insert(missing, id)
        end
    end
    table.sort(missing)
    return missing
end

function make_cppwinrt_args(out_dir, inputs, refs)
    local args = { "-out", out_dir, "-optimize", "-prefix", "-overwrite" }
    util.append_args(args, "-in", inputs)
    util.append_args(args, "-ref", refs)
    return args
end

function target_context()
    local project_dir = os.projectdir()
    local packages_config = path.join(project_dir, "packages.config")
    local packages_dir = path.join(project_dir, ".xmake", "nuget")
    local missing = missing_packages(packages_dir)
    local nuget = util.find_nuget_from_env()
    if #missing > 0 and not nuget then
        util.fail("nuget.exe not found from environment variables or PATH")
    end

    local sdk_root = util.sdk_root(util.fail)
    local include_dir = util.latest_dir(path.join(sdk_root, "Include", "10.*"))
    if not include_dir then
        util.fail("failed to find Windows SDK Include directory")
    end

    local sdk_ref_dir = util.latest_dir(path.join(sdk_root, "References", "10.*"))
    if not sdk_ref_dir then
        util.fail("failed to find Windows SDK References directory")
    end
    local sdk_version = path.filename(sdk_ref_dir)

    local platform_refs = util.platform_winmds(sdk_root, sdk_version)
    local appsdk_path = util.package_path(packages_dir, "Microsoft.WindowsAppSDK", PACKAGE_VERSIONS, util.fail)
    local appsdk_lib = path.join(appsdk_path, "lib")
    local buildtools_bindir = util.sdk_buildtools_bindir(packages_dir, PACKAGE_VERSIONS, util.fail)
    local foundation_metadata_dir = util.find_foundation_metadata_dir(sdk_root, sdk_version, util.fail)

    local ver_uap_dirs = { "uap10.0.17763", "uap10.0.18362", "uap10.0.19041", "uap10.0" }
    local base_uap_dirs = { "uap10.0" }
    local appsdk_winmds = {
        util.appsdk_winmd_path(appsdk_lib, ver_uap_dirs, "Microsoft.Foundation"),
        util.appsdk_winmd_path(appsdk_lib, ver_uap_dirs, "Microsoft.Graphics"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.UI.Text"),
        util.appsdk_winmd_path(appsdk_lib, ver_uap_dirs, "Microsoft.UI"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.UI.Xaml"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.ApplicationModel.DynamicDependency"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.ApplicationModel.Resources"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.ApplicationModel.WindowsAppRuntime"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.AppLifecycle"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.AppNotifications.Builder"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.AppNotifications"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.Management.Deployment"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.PushNotifications"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.Security.AccessControl"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.System.Power"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.System"),
        util.appsdk_winmd_path(appsdk_lib, base_uap_dirs, "Microsoft.Windows.Widgets")
    }

    local webview2_winmd = path.join(util.package_path(packages_dir, "Microsoft.Web.WebView2", PACKAGE_VERSIONS, util.fail), "lib",
        "Microsoft.Web.WebView2.Core.winmd")

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

    local core_platform_refs = util.filter_core_platform_winmds(platform_refs)

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
        cppwinrt = path.join(util.package_path(packages_dir, "Microsoft.Windows.CppWinRT", PACKAGE_VERSIONS, util.fail), "bin", "cppwinrt.exe"),
        implementation_include = path.join(util.package_path(packages_dir, "Microsoft.Windows.ImplementationLibrary", PACKAGE_VERSIONS, util.fail),
            "include"),
        appsdk_include = path.join(appsdk_path, "include"),
        appsdk_libdir = path.join(appsdk_path, "lib", "win10-x64"),
        appsdk_tools_dir = path.join(appsdk_path, "tools"),
        xaml_compiler = path.join(appsdk_path, "tools", "net472", "XamlCompiler.exe"),
        midl = path.join(buildtools_bindir, "midl.exe"),
        mdmerge = path.join(buildtools_bindir, "mdmerge.exe"),
        makepri = path.join(buildtools_bindir, "makepri.exe"),
        foundation_metadata_dir = foundation_metadata_dir,
        bootstrap_dll = path.join(appsdk_path, "runtimes", "win-x64", "native",
            "Microsoft.WindowsAppRuntime.Bootstrap.dll"),
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
        xaml_apps = { app_xaml },
        xaml_pages = { mainwindow_xaml },
        xaml_saved_state = path.join(generated_xaml, "XamlCompilerState.xml"),
        xaml_pass1_in_json = path.join(xmake_root, "xaml.pass1.in.json"),
        xaml_pass1_out_json = path.join(xmake_root, "xaml.pass1.out.json"),
        xaml_pass2_in_json = path.join(xmake_root, "xaml.pass2.in.json"),
        xaml_pass2_out_json = path.join(xmake_root, "xaml.pass2.out.json"),
        xaml_xbf_files = { "src/App.xbf", "src/MainWindow.xbf" },
        makepri_config = path.join(xmake_root, "resources.pri.xml"),
        makepri_resfiles = path.join(xmake_root, "resources.pri.resfiles"),
        pri_file = path.join(generated_xaml, "resources.pri")
    }
end
