-- WinUIEdit XAML 主题与原生运行时集成规则。
--
-- 本规则将默认主题 XAML 纳入 XAML 编译，并在 after_build 生命周期
-- 将匹配目标架构的 WinUIEditor.dll 与 WinUIEditor.pri 复制到目标输出目录。
--
-- 用法（目标 xmake.lua）：
--     add_rules("winuiedit")

rule("winuiedit")
    add_deps("winui3.env")
    add_orders("winui3.xaml", "winuiedit")
    add_orders("winuiedit", "winui3.pri")

    after_load(function (target)
        target:add("files", path.join(
            target:autogendir({root = true}),
            "generated", "WinUIEditor", "Themes", "Generic.xaml"
        ), {always_added = true})
    end)

    on_config(function (target)
        local packages = import("winui3.packages")
        local generic_xaml = path.join(
            packages.package_root("WinUIEdit"),
            "lib", "uap10.0", "WinUIEditor", "Themes", "Generic.xaml"
        )
        local generated_xaml = path.join(
            target:autogendir({root = true}),
            "generated", "WinUIEditor", "Themes", "Generic.xaml"
        )

        os.mkdir(path.directory(generated_xaml))
        os.cp(generic_xaml, generated_xaml, {copy_if_different = true})
    end)

    before_build(function (target)
        local generated_dir = path.join(target:autogendir({root = true}), "generated")
        local xbf_src = path.join(generated_dir, "Generic.xbf")
        local xbf_dst = path.join(generated_dir, "WinUIEditor", "Themes", "Generic.xbf")
        if os.isfile(xbf_src) then
            -- XAML 编译器会忽略输入目录层级，需恢复库资源路径。
            os.rm(xbf_dst)
            os.mkdir(path.directory(xbf_dst))
            os.mv(xbf_src, xbf_dst)
        end
    end)

    after_build(function (target)
        local arch = target:arch()
        if arch ~= "x64" and arch ~= "x86" and arch ~= "arm64" then
            raise("winuiedit: 不支持的目标架构：" .. tostring(arch)
                .. "；仅支持 x64、x86、arm64。")
        end

        local packages = import("winui3.packages")
        local runtime_dir = path.join(
            packages.package_root("WinUIEdit"),
            "runtimes", "win10-" .. arch, "native"
        )

        os.cp(path.join(runtime_dir, "WinUIEditor.dll"), target:targetdir(),
            {copy_if_different = true})
        os.cp(path.join(runtime_dir, "WinUIEditor.pri"), target:targetdir(),
            {copy_if_different = true})
    end)
