-- winui3.xmdp 规则：XamlMetaDataProvider 自动生成
--
-- 本规则为每个目标生成 XamlMetaDataProvider.idl 与 XamlMetaDataProvider.cpp，
-- 并将生成的 .cpp 注册至目标编译流程。依赖 winui3.env 规则校验 namespace，
-- 生成目录通过 target:autogendir() 按当前目标即时计算。
--
-- 生命周期说明：
--   1. after_load（加载阶段）：将生成 .cpp 注册至目标源文件
--      （always_added=true，因文件此时尚未生成）。
--   2. on_config（配置阶段）：确保目录存在，以内容感知方式写入 .idl 与 .cpp，
--      仅当内容变化时才更新文件 mtime。
rule("winui3.xmdp")
    add_deps("winui3.env")

    after_load(function (target)
        import("xmdp").after_load(target)
    end)

    on_config(function (target)
        import("xmdp").on_config(target)
    end)
