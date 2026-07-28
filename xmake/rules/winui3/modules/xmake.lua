-- winui3.modules 规则：为应用目标启用 C++ modules 模式
--
-- 设置 build.c++.modules 策略，依赖 winui3.shared_projection.modules 共享模块目标，
-- 并在 on_config 中设置 WINRT_ENABLE_LEGACY_COM 和 WINUI3_IMPORT_MODULE 宏。
--
-- 使用方式：
--   add_rules("winui3.app", "winui3.modules")
--   set_values("winui3.namespace", "<namespace>")
--
-- WINUI3_IMPORT_MODULE 由 namespace 自动派生为 <namespace>.winrt。
rule("winui3.modules")
    on_load(function (target)
        local modules = import("modules")
        modules.on_load(target)
    end)

    on_config(function (target)
        local modules = import("modules")
        modules.on_config(target)
    end)
