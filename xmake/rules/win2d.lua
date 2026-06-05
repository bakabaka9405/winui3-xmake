-- xmake rule for Win2D (Microsoft.Graphics.Win2D) native DLL deployment
--
-- 本规则在 after_build 生命周期将 Microsoft.Graphics.Canvas.dll
-- 复制到目标输出目录。仅当目标显式添加 win2d 规则时才执行部署。
--
-- Usage (in target xmake.lua):
--     add_rules("win2d")

rule("win2d")
    after_build(function (target)
        local packages = import("winui3.packages")

        local win2d_root = packages.package_root("Microsoft.Graphics.Win2D")

        local canvas_src = path.join(
            win2d_root,
            "runtimes", "win-x64", "native",
            "Microsoft.Graphics.Canvas.dll"
        )

        local canvas_dst = path.join(target:targetdir(), "Microsoft.Graphics.Canvas.dll")
        os.cp(canvas_src, canvas_dst, {copy_if_different = true})
    end)
