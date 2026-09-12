-- winui3.xaml_lsp.autoupdate 规则：在目标构建成功后发布 XAML LSP 元数据清单
--
-- 作为 project 级规则在全局 build 成功后（_do_project_rules("build_after")）调用，
-- 确保在目标产物编译完成后发布最新的清单文件。

rule("winui3.xaml_lsp.autoupdate")
    set_kind("project")

    after_build(function (opt)
        if opt and opt.errors then
            return
        end
        import("winui3.manifest").export()
    end)
