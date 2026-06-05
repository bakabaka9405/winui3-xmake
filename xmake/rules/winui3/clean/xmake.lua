-- winui3.clean 规则：清理生成文件
--
-- 在 xmake clean 之后执行，清理 WinUI3 构建流水线产生的中间文件和部署文件。
-- 生成文件位于 build/.gens/<target>/ 目录，部署文件位于 build/windows/x64/<mode>/<target>/ 目录。
--
-- 本规则依赖 winui3.env 提供正确的 targetdir 布局，不可独立使用。
-- 使用方式：通过 winui3.app 聚合规则自动包含。

rule("winui3.clean")
    add_deps("winui3.env")

    after_clean(function (target)
        import("clean").after_clean(target)
    end)