-- xmake rule for WebView2 (Microsoft.Web.WebView2) native DLL deployment
--
-- 本规则在 after_build 生命周期将 WebView2 原生 DLL 与 Web 前端资源
-- 复制到目标输出目录。仅当目标显式添加 webview 规则时才执行部署。
--
-- Usage (in target xmake.lua):
--     add_rules("webview", {dist_dir = path.join(os.scriptdir(), "src", "web")})
--
-- dist_dir 是必需参数，必须显式指定 Web 前端资源目录路径。

rule("webview")
    on_config(function (target)
        local dist_dir = target:extraconf("rules", "webview", "dist_dir")
        if type(dist_dir) ~= "string" then
            raise("webview: requires \"dist_dir\" parameter.\n"
                .. "Usage: add_rules(\"webview\", "
                .. "{dist_dir = path.join(os.scriptdir(), \"src\", \"web\")})")
        end
        if not os.isdir(dist_dir) then
            raise(string.format(
                "webview: dist_dir directory does not exist: %s\n"
                .. "  ensure dist_dir points to a directory containing web frontend resources (HTML/CSS/JS).",
                dist_dir))
        end
        local web_src_index = path.join(dist_dir, "index.html")
        if not os.isfile(web_src_index) then
            raise(string.format(
                "webview: index.html not found in dist_dir: %s\n"
                .. "  dist_dir must contain index.html as the web frontend entry point.",
                web_src_index))
        end
    end)

    after_build(function (target)
        local packages = import("winui3.packages")

        local webview2_root = packages.package_root("Microsoft.Web.WebView2")

        local dist_dir = target:extraconf("rules", "webview", "dist_dir")

        local core_src = path.join(
            webview2_root,
            "runtimes", "win-x64", "native_uap",
            "Microsoft.Web.WebView2.Core.dll"
        )
        local core_dst = path.join(target:targetdir(), "Microsoft.Web.WebView2.Core.dll")

        os.cp(core_src, core_dst, {copy_if_different = true})
        os.cp(dist_dir, target:targetdir(), {copy_if_different = true})

        cprint("${bright cyan}WebView2:${clear} Evergreen Runtime required — built into Windows 11; separate install needed on Windows 10")
        cprint("${bright cyan}WebView2:${clear} Download: https://developer.microsoft.com/microsoft-edge/webview2/")
    end)
