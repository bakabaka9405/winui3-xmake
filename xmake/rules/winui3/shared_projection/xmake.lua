-- winui3.shared_projection 规则：显式共享 C++/WinRT 投影头生成
--
-- 本规则从平台 WinMD、Windows App SDK WinMD、WebView2 WinMD 和 Win2D WinMD
-- 生成所有目标共用的 C++/WinRT 投影头，输出至 build/.gens/shared/generated 目录。使用 xmake 依赖缓存（core.project.depend）实现增量生成。
--
-- 规则通过 winui3.winmd.context 暴露 WinMD 上下文供下游规则消费。
rule("winui3.shared_projection")
    before_prepare(function (target)
        import("shared_projection").before_prepare(target)
    end)

target("winui3.shared_projection.modules")
    set_kind("static")
    set_default(false)
    set_languages("cxxlatest")
    set_policy("build.c++.modules", true)

    -- 各个模块的 WINUI3_IMPORT_MODULE 宏不一致，启用严格策略会导致该公共 target 的产物无法复用，之后再想办法解决这个问题
    -- set_policy("build.c++.modules.reuse.strict", true)

    set_policy("build.fence", true)
    add_files(path.join(os.projectdir(), "build", ".gens", "shared", "generated", "winrt", "*.ixx"), {always_added = true, public = true})

    on_config(function (target)
        import("core.project.task").run("nuget-check")

        local packages = import("winui3.packages")
        for _, nuget_id in ipairs(packages.all_packages()) do
            local pkg_root = packages.package_root(nuget_id)
            local include_dir = path.join(pkg_root, "include")
            if os.isdir(include_dir) then
                target:add("includedirs", include_dir)
            end
        end

        target:add("includedirs", path.join(os.projectdir(), "build", ".gens", "shared", "generated"))
        target:add("cxflags", "/EHsc", "/bigobj", "/await:strict", "/utf-8")
        target:add("defines", "NOMINMAX", "WIN32_LEAN_AND_MEAN", "UNICODE", "_UNICODE")
        target:add("defines", "WINRT_ENABLE_LEGACY_COM")

        import("shared_projection").before_prepare(target, {modules_target = true})
    end)
