-- xmake plugin: nuget-check
-- 验证 NuGet 包环境完整性：解析 packages.config，检查所有包是否存在于全局包缓存中，
-- 缺失时自动执行 nuget restore。可作为 CLI 工具或通过 task.run("nuget-check") 调用。
--
-- 用法:
--   xmake nuget-check          (CLI 手动检查)
--   task.run("nuget-check")    (规则 / on_config 中调用)

local _nuget_check_done = false

task("nuget-check")
    set_category("plugin")

    on_run(function ()
        if _nuget_check_done then
            return
        end

        local option = import("core.base.option")
        local xml    = import("core.base.xml")

        -- 阶段 1：解析 packages.config
        local config_path = path.join(os.projectdir(), "packages.config")
        if not os.isfile(config_path) then
            raise("packages.config 未找到：" .. config_path)
        end

        local doc  = xml.loadfile(config_path)
        local root = xml.find(doc, "/packages")
        if not root then
            raise("packages.config 中未找到 <packages> 根元素")
        end

        local packages = {}
        for _, pkg in ipairs(root.children or {}) do
            if pkg.name == "package" then
                local pid = pkg.attrs and pkg.attrs.id
                local pver = pkg.attrs and pkg.attrs.version
                if pid and pver then
                    table.insert(packages, {id = pid, version = pver})
                end
            end
        end

        if #packages == 0 then
            raise("packages.config 中未找到任何 <package> 声明")
        end

        -- 阶段 2：定位 NuGet 全局包缓存目录
        local function resolve_nuget_global_dir()
            local candidates = {}

            local env_dir = os.getenv("NUGET_PACKAGES")
            if env_dir and #env_dir > 0 then
                table.insert(candidates, {label = "NUGET_PACKAGES", dir = env_dir})
            end

            local userprofile = os.getenv("USERPROFILE")
            if userprofile and #userprofile > 0 then
                table.insert(candidates, {
                    label = "%USERPROFILE%/.nuget/packages",
                    dir   = path.join(userprofile, ".nuget", "packages")
                })
            end

            local dotnet_tool = import("lib.detect.find_tool")("dotnet")
            if dotnet_tool and dotnet_tool.program then
                local dotnet_result = os.iorunv(dotnet_tool.program,
                    {"nuget", "locals", "global-packages", "--list"})
                if dotnet_result then
                    local dir = dotnet_result:match("global%-packages:%s*(.+)")
                    if dir then
                        dir = dir:gsub("%s+$", "")
                        table.insert(candidates, {
                            label = "dotnet nuget locals",
                            dir   = dir
                        })
                    end
                end
            end

            for _, c in ipairs(candidates) do
                if os.isdir(c.dir) then
                    if option.get("verbose") then
                        cprint("${dim}NuGet 全局包缓存: %s (%s)", c.dir, c.label)
                    end
                    return c.dir, c.label
                end
            end

            return nil, nil
        end

        local packagesdir = resolve_nuget_global_dir()
        if not packagesdir then
            raise("无法定位 NuGet 全局包缓存目录。\n"
                .. "  请设置 NUGET_PACKAGES 环境变量或确认 .nuget/packages 目录存在。")
        end

        -- 阶段 3：检查所有包是否存在
        local missing = {}
        for _, pkg in ipairs(packages) do
            local pkg_dir = path.join(packagesdir, string.lower(pkg.id), pkg.version)
            if not os.isdir(pkg_dir) then
                table.insert(missing, pkg)
            end
        end

        if #missing == 0 then
            _nuget_check_done = true
            if option.get("verbose") then
                cprint("${color.success}NuGet 环境检查通过：全部 %d 个包已在缓存中。", #packages)
            end
            return
        end

        -- 阶段 4：自动恢复缺失的包
        local missing_names = {}
        for _, pkg in ipairs(missing) do
            table.insert(missing_names, string.format("  %s %s", pkg.id, pkg.version))
        end
        cprint("${color.warning}NuGet 全局缓存中缺失 %d/%d 个包:", #missing, #packages)
        cprint(table.concat(missing_names, "\n"))

        local nuget_prog = nil
        local find_tool = import("lib.detect.find_tool")
        if find_tool then
            local tool = find_tool("nuget", {check = "help"})
            if tool and tool.program then
                nuget_prog = tool.program
            end
        end

        if not nuget_prog then
            raise("无法定位 nuget 可执行文件。请安装 NuGet CLI 或确保 lib.detect.find_tool 可发现 nuget。")
        end

        cprint("${color.warning}正在执行 nuget restore ...")
        os.runv(nuget_prog, {"restore", "packages.config",
            "-PackagesDirectory", packagesdir})

        -- 阶段 5：恢复后二次验证
        local still_missing = {}
        for _, pkg in ipairs(packages) do
            local pkg_dir = path.join(packagesdir, string.lower(pkg.id), pkg.version)
            if not os.isdir(pkg_dir) then
                table.insert(still_missing, pkg)
            end
        end

        if #still_missing > 0 then
            local names = {}
            for _, pkg in ipairs(still_missing) do
                table.insert(names, string.format("  %s %s", pkg.id, pkg.version))
            end
            raise("nuget restore 完成后仍有 %d 个包缺失:\n"
                .. table.concat(names, "\n")
                .. "\n\n  请手动执行 nuget restore 或检查网络连接与 NuGet 源配置。",
                #still_missing)
        end

        _nuget_check_done = true
        cprint("${color.success}NuGet 环境检查通过：全部 %d 个包已就绪。", #packages)
    end)

    set_menu {
        usage       = "xmake nuget-check",
        description = "验证 NuGet 包环境完整性，缺失时自动执行 nuget restore。",
        options     = {}
    }
