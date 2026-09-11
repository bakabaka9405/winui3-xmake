-- winui3.shared_projection 规则：共享 C++/WinRT 投影生成
--
-- 投影按 WinMD 节点独立输出到 build/.gens/shared/<node>/generated。
--
-- 普通目标由本规则生成所需节点的投影头；C++ modules 目标依赖下方与 WinMD
-- 节点一一对应的 winui3.shared_projection.modules.<node> 静态目标。
--
-- 规则通过 winui3.winmd.context 暴露 WinMD 上下文供下游规则消费。
rule("winui3.shared_projection")
    before_prepare(function (target)
        import("shared_projection").before_prepare(target)
    end)

function _shared_modules_target(node_id, deps)
    local name = "winui3.shared_projection.modules." .. node_id

    target(name)
        set_kind("static")
        set_default(false)
        set_languages("cxxlatest")
        set_policy("build.c++.modules", true)
        set_values("winui3.shared_projection.node", node_id)

        add_files(path.join(os.projectdir(), "build", ".gens", "shared", node_id,
            "generated", "winrt", "*.ixx"), {always_added = true, public = true})

        -- 不启用 build.c++.modules.reuse.strict：消费目标的 WINUI3_IMPORT_MODULE 定义不同，
        -- 严格检查会拒绝共享同一份节点 BMI。

        set_policy("build.fence", true)

        -- target 依赖与 WinMD 图边保持一致。
        for _, dep_id in ipairs(deps or {}) do
            add_deps("winui3.shared_projection.modules." .. dep_id)
        end

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

            local winmd_context = import("winui3.winmd.context")
            local shared = winmd_context.ensure(target)
            for _, active_id in ipairs(shared.nodes) do
                target:add("includedirs", winmd_context.node_dir(active_id))
            end

            target:add("cxflags", "/EHsc", "/bigobj", "/await:strict", "/utf-8")
            target:add("defines", "NOMINMAX", "WIN32_LEAN_AND_MEAN", "UNICODE", "_UNICODE")
            target:add("defines", "WINRT_ENABLE_LEGACY_COM")

            import("shared_projection").generate(target, {
                node    = node_id,
                modules = true,
            })
        end)
end

-- xmake 描述阶段不提供 import；这里显式保持与 WinMD 图相同的节点和边。
_shared_modules_target("platform")
_shared_modules_target("webview2", {"platform"})
_shared_modules_target("appsdk", {"platform", "webview2"})
_shared_modules_target("winuiedit", {"platform", "webview2", "appsdk"})
_shared_modules_target("win2d", {"platform", "webview2", "appsdk"})
