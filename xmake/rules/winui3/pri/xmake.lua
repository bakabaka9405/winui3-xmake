-- winui3.pri 规则：资源索引与 PRI 生成
--
-- 本规则消费 winui3.xaml 规则生成的 .xbf 文件，
-- 通过 makepri.exe 生成 layout.resfiles、priconfig.xml 和 resources.pri。
--
-- 回调模型：
--   - before_build:        增量 PRI 生成（在 XAML before_build 之后）
--
-- 依赖上游规则：
--   - winui3.env:            namespace 校验与目标编译环境
--   - winui3.xaml:           XAML 编译
rule("winui3.pri")
    add_deps("winui3.env", "winui3.xaml")
    add_orders("winui3.xaml", "winui3.pri")

    before_build(function (target)
        import("pri").before_build(target)
    end)
