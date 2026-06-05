local packages = import("winui3.packages")

function after_build(target)
    local outdir = target:targetdir()
    local target_name = target:name()

    local pri_src = path.join(target:autogendir({root = true}), "generated", "resources.pri")
    local pri_dst = path.join(target:targetdir(), "resources.pri")

    os.cp(pri_src, pri_dst, {copy_if_different = true})

    local foundation_root = packages.package_root("Microsoft.WindowsAppSDK.Foundation")

    local dll_src = path.join(foundation_root, "runtimes", "win-x64", "native",
        "Microsoft.WindowsAppRuntime.Bootstrap.dll")
    local dll_dst = path.join(target:targetdir(), "Microsoft.WindowsAppRuntime.Bootstrap.dll")

    os.cp(dll_src, dll_dst, {copy_if_different = true})
end

