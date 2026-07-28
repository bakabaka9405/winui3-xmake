local utils = import("utils")
local task = import("core.project.task")

function on_load(target)
    target:set("kind", "binary")
    target:set("targetdir", path.join(target:targetdir(), target:name()))
    target:set("rundir", target:targetdir())
    target:set("policy", "build.across_targets_in_parallel", false)
end

function before_prepare(target)
end

function on_config(target)
    local namespace = target:values("winui3.namespace")
    if namespace == nil or type(namespace) ~= "string" then
        raise("winui3.app requires \"namespace\" parameter.\n")
    end

    -- NuGet 环境验证：确保 packages.config 中声明的包已安装在全局缓存中。
    -- nuget-check 在单次 xmake 进程内只执行一次实际检查。
    task.run("nuget-check")
    local packages = import("winui3.packages")

    for _, nuget_id in ipairs(packages.all_packages()) do
        local pkg_root = packages.package_root(nuget_id)
        local include_dir = path.join(pkg_root, "include")
        if os.isdir(include_dir) then
            target:add("includedirs", include_dir)
        end
    end

    local autogen_root = target:autogendir({root = true})
    local generated_dir = path.join(autogen_root, "generated")

    os.mkdir(generated_dir)
    os.mkdir(path.join(autogen_root, "winmd_unmerged"))
    os.mkdir(path.join(autogen_root, "winmd_merged"))

    target:add("includedirs", generated_dir)
    target:add("includedirs", path.join(os.projectdir(), "build", ".gens", "shared", "generated"))

    target:add("cxflags", "/EHsc", "/bigobj", "/await:strict", "/utf-8")

    target:add("defines", "NOMINMAX", "WIN32_LEAN_AND_MEAN", "UNICODE", "_UNICODE")
    target:add("defines", "DISABLE_XAML_GENERATED_MAIN")
    target:add("defines", "WINUI3_APP_NAMESPACE=" .. namespace)

    local foundation_root = packages.package_root("Microsoft.WindowsAppSDK.Foundation")
    target:add("linkdirs", path.join(foundation_root, "lib", "native", "x64"))

    target:add("links", "Microsoft.WindowsAppRuntime.Bootstrap")
    target:add("links", "windowsapp")
    target:add("links", "user32")

    target:add("ldflags", "/SUBSYSTEM:WINDOWS")
end

