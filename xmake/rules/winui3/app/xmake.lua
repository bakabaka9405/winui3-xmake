-- winui3.app 规则：WinUI3 应用顶层组合规则
--
-- 通过 add_deps 聚合全部 8 个子规则，形成完整的 WinUI3 构建流水线
--
-- 子规则拓扑（按构建顺序）：
--   winui3.env              — 目标身份、参数校验、包/编译/链接环境
--   winui3.shared_projection — 共享 C++/WinRT 投影头生成（所有目标共用）
--   winui3.xmdp             — XamlMetaDataProvider.idl/.cpp 自动生成
--   winui3.idl              — MIDL 编译、WinMD 合并、项目 C++/WinRT 投影
--   winui3.xaml             — XAML 编译（Pass 1 → .xbf, Pass 2 → .g.hpp/.g.cpp）
--   winui3.pri              — 资源索引与 PRI 生成（makepri）
--   winui3.deployment       — 运行时文件部署（Bootstrap.dll + resources.pri）
--   winui3.clean            — 清理生成文件（after_clean）
--
-- 使用方式：
--   add_rules("winui3.app")
--   set_values("winui3.namespace", "hello")
rule("winui3.app")
    add_deps("winui3.env",
             "winui3.shared_projection",
             "winui3.xmdp",
             "winui3.idl",
             "winui3.xaml",
             "winui3.pri",
             "winui3.deployment",
             "winui3.clean")


