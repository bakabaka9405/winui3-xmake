-- WinUI3 子规则按依赖拓扑加载；具体执行顺序由各规则的 add_deps/add_orders 声明。
-- 子规则拓扑：
--   winui3.env              — NuGet 环境与工具链探测
--   winui3.shared_projection — 共享 C++/WinRT 投影头生成
--   winui3.xmdp             — XamlMetaDataProvider 自动生成
--   winui3.idl              — MIDL 编译与 WinMD 合并
--   winui3.xaml             — XAML 编译（Pass1/Pass2）
--   winui3.pri              — 资源索引生成
--   winui3.deployment       — 运行时文件部署
--   winui3.clean            — 清理生成文件
--   winui3.app              — 应用组合规则（通过 add_deps 聚合上述所有规则）
--
includes("winui3/modules/xmake.lua")
includes("winui3/env/xmake.lua")
includes("winui3/shared_projection/xmake.lua")
includes("winui3/xmdp/xmake.lua")
includes("winui3/idl/xmake.lua")
includes("winui3/xaml/xmake.lua")
includes("winui3/pri/xmake.lua")
includes("winui3/deployment/xmake.lua")
includes("winui3/clean/xmake.lua")
includes("winui3/app/xmake.lua")
