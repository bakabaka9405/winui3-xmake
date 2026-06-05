-- winui3.shared_projection 规则：显式共享 C++/WinRT 投影头生成
--
-- 本规则从平台 WinMD、Windows App SDK WinMD、WebView2 WinMD 和 Win2D WinMD
-- 生成所有目标共用的 C++/WinRT 投影头，输出至 build/.gens/shared/generated 目录。使用 xmake 依赖缓存（core.project.depend）实现增量生成。
--
-- 规则通过 winui3.winmd.context 暴露 WinMD 上下文供下游规则消费。
rule("winui3.shared_projection")
    on_prepare(function (target)
        import("shared_projection").on_prepare(target)
    end)
