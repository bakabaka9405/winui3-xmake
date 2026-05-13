-- xmake plugin: nuget-check
-- 验证 NuGet 包环境完整性：检查 packages.config 中声明的所有包是否存在于
-- 全局包缓存目录，缺失时自动执行 nuget restore。
--
-- 既作为独立 CLI 工具使用，也通过 task.run("nuget-check") 在构建流程中调用。
-- Usage:
--   xmake nuget-check                                (CLI 手动检查)
--   task.run("nuget-check")                          (规则/任务中调用)
--
-- 沙箱说明：本插件使用 os.run 执行 nuget restore（非 os.execv），
-- 因为 nuget restore 需要捕获输出以确保环境完整性，而不仅仅是执行。

task("nuget-check")
    set_category("plugin")

    on_run(function ()
        -- ── 依赖导入 ─────────────────────────────────────────────
        -- 导入必须在回调内部进行，以适配 xmake 沙箱隔离机制。
        local nuget_cfg = import("scripts.nuget_config", {rootdir = os.projectdir()})
        local option = import("core.base.option")
        
        local utils = import("utils")

        -- ── 环境完整性检查 ─────────────────────────────────────
        -- 验证 packages.config 中所有包的本地目录是否真实存在。
        local env_ok, missing = nuget_cfg.check_env()

        if not env_ok then
            -- 第一阶段警告：列出缺失包
            utils.vcprint("${color.warning}NuGet packages missing from global cache: %s",
                   table.concat(missing, ", "))

            -- 验证 USERPROFILE 环境变量（nuget restore 依赖于全局包目录路径）
            local userprofile = os.getenv("USERPROFILE")
            if not userprofile then
                raise("USERPROFILE environment variable is not set; cannot run nuget restore")
            end

            -- 自动执行 nuget restore
            local nuget_packages_dir = userprofile .. "\\.nuget\\packages"
            utils.vcprint("${color.warning}Running 'nuget restore' to install missing packages...")
            os.run("nuget restore -PackagesDirectory \"%s\"", nuget_packages_dir)

            -- 清空 nuget_config 模块缓存，强制重新解析 packages.config
            nuget_cfg.reset_cache()

            -- 恢复后第二次验证
            local env_ok2, missing2 = nuget_cfg.check_env()
            if not env_ok2 then
                raise("NuGet packages still missing after 'nuget restore':\n  "
                      .. table.concat(missing2, "\n  "))
            end

            utils.vcprint("${color.success}All NuGet packages verified.")
        else
            local _, package_ids = nuget_cfg.all_packages()
            utils.vcprint("${color.success}NuGet environment check passed: all %d packages found.",
                   #package_ids)
        end
    end)

    set_menu {
        usage       = "xmake nuget-check",
        description = "Verify NuGet package environment and restore missing packages if needed.",
        options     = {}
    }
