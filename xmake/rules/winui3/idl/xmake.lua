-- winui3.idl 规则：MIDL 编译、WinMD 合并与项目 C++/WinRT 投影
--
-- 本规则声明 set_extensions(".idl")，处理项目 .idl 文件和生成的 XMDP .idl 文件，
-- 执行完整的 MIDL → mdmerge → cppwinrt 管线。
--
-- 回调模型（遵循 xmake platform.windows.idl 参考模式）：
--   - before_prepare_files: 生成项目 WinMD 与 C++/WinRT 投影，供后续 XAML 与模块扫描使用
--
-- 依赖上游规则：
--   - winui3.env:              namespace 校验与目标编译环境
--   - winui3.shared_projection: 共享投影头目录、平台 WinMD 引用
--   - winui3.xmdp:             XamlMetaDataProvider 生成（执行顺序依赖）
rule("winui3.idl")
    add_deps("winui3.env", "winui3.shared_projection", "winui3.xmdp")
    set_extensions(".idl")

    on_load(function (target)
        local builtin = target:rule("platform.windows.idl")
        if builtin then
            builtin:set("extensions", "")
        end
    end)

    after_load(function (target)
        import("idl").after_load(target)
    end)

    before_prepare_files(function (target, sourcebatch, opt)
        import("idl").before_prepare_files(target, sourcebatch, opt)
    end)
