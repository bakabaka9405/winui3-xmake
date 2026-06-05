-- winui3.deployment 规则：运行时文件部署
-- 依赖 winui3.pri 提供 resources.pri，并从 Foundation NuGet 包解析 Bootstrap.dll。
-- 本规则不复制 WebView2 / Win2D 文件 — 这些由可选的 webview / win2d 规则各自处理。
rule("winui3.deployment")
    add_deps("winui3.pri")
    add_orders("winui3.pri", "winui3.deployment")

    after_build(function (target)
        import("deployment").after_build(target)
    end)
