-- winui3.xaml 规则：XAML 编译（Pass1 / Pass2）
--
-- 本规则声明 set_extensions(".xaml")，处理项目中的 .xaml 文件，
-- 执行 XAML Compiler Pass 1（生成 .xbf）与 Pass 2（生成 .g.hpp / .g.cpp）。
--
-- 回调模型（C++/WinRT .g.cpp inclusion 模式）：
--   - before_build:  依赖跟踪 + XAML Pass 1（生成 .xbf）+ Pass 2（生成 .g.hpp / .g.cpp）
--   - .g.cpp 不编译为独立单元；由 *.xaml.cpp 和 XamlMetaDataProvider.cpp 通过 #include 包含
--
-- 依赖上游规则：
--   - winui3.env:              namespace 校验与目标编译环境
--   - winui3.shared_projection: 共享投影头目录、平台 WinMD 引用
--   - winui3.idl:              MIDL / WinMD 合并管线（执行顺序依赖）

rule("winui3.xaml")
    add_deps("winui3.env", "winui3.shared_projection", "winui3.idl")
    set_extensions(".xaml")

    before_build(function (target)
        import("xaml").before_build(target)
    end)
