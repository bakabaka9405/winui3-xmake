-- xmake rule for WebView2 (Microsoft.Web.WebView2) native DLL deployment
--
-- This rule copies the WebView2 loader and WinUI/UAP native component from the
-- NuGet package to the target output directory after build. Only add this rule
-- to targets that actually depend on WebView2; targets without WebView2 should
-- not include this rule to avoid unnecessary DLL copies.
--
-- Usage (in target xmake.lua):
--     add_rules("webview", {dist_dir = path.join(os.scriptdir(), "src", "web")})

local _cached_webview2_path = nil

rule("webview")
    after_build(function (target)
        -- Resolve WebView2 NuGet package path from packages.config (cached at module level)
        if not _cached_webview2_path then
            local nuget_cfg = import("scripts.nuget_config", {rootdir = os.projectdir()})
            _cached_webview2_path = nuget_cfg.package_path("Microsoft.Web.WebView2")
        end

        if not _cached_webview2_path then
            raise("Microsoft.Web.WebView2 NuGet package not found in packages.config")
        end

        -- Copy WebView2 native DLLs to target output directory
        local outdir = target:targetdir()
        local core_src = (_cached_webview2_path .. "/runtimes/win-x64/native_uap/Microsoft.Web.WebView2.Core.dll"):gsub("/", "\\")
        local core_dst = path.join(outdir, "Microsoft.Web.WebView2.Core.dll")
        if not os.isfile(core_src) then
            raise("Microsoft.Web.WebView2.Core.dll not found at " .. core_src)
        end
        os.cp(core_src, core_dst)

        -- Copy web/ resources (HTML, JS, CSS) from source to output directory
        local dist_dir = target:extraconf("rules", "webview", "dist_dir")
        if type(dist_dir) ~= "string" then
            raise("webview requires \"dist_dir\" parameter.\nUsage: add_rules(\"webview\", {dist_dir = path.join(os.scriptdir(), \"src\", \"web\")})")
        end
        if not os.isdir(dist_dir) then
            raise("webview: dist_dir directory not found: " .. dist_dir)
        end
        local web_src_index = path.join(dist_dir, "index.html")
        if not os.isfile(web_src_index) then
            raise("demo.webview: index.html not found at " .. web_src_index)
        end
        os.cp(dist_dir, outdir)

        cprint("${bright cyan}WebView2:${clear} Evergreen Runtime required — built into Windows 11; separate install needed on Windows 10")
        cprint("${bright cyan}WebView2:${clear} Download: https://developer.microsoft.com/microsoft-edge/webview2/")
    end)
