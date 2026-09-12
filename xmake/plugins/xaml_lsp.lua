-- xmake plugin: xaml-lsp
-- 导出 XAML LSP 元数据清单 (.xmake/xaml-lsp.json)
--
-- 用法:
--   xmake xaml-lsp

task("xaml-lsp")
    set_category("plugin")

    on_run(function ()
        local task = import("core.project.task")
        task.run("config", {}, {loadonly = true})

        local manifest_mod = import("winui3.manifest")
        manifest_mod.export()
        cprint("${color.success}XAML LSP manifest published: .xmake/xaml-lsp.json")
    end)

    set_menu {
        usage       = "xmake xaml-lsp",
        description = "Export XAML LSP manifest (.xmake/xaml-lsp.json)",
        options     = {}
    }
