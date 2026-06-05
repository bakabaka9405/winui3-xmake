-- 工具与 WinMD 解析均通过模块（winui3.tools / winui3.winmd.* / winui3.packages）。
--
-- WinMD 上下文由 winui3.winmd.context 持有，供下游规则复用。
-- 共享投影头按拓扑序逐节点生成：每个节点 WinMD 作为 -in，已处理节点的 WinMD 作为 -ref。


local depend = import("core.project.depend")
local winmd_context = import("winui3.winmd.context")
local option = import("core.base.option")
local config = import("core.project.config")

function on_prepare(target)
    local shared = winmd_context.ensure(target)
    local graph_ctx = shared.graph

    local tools = import("winui3.tools")()

    -- 收集所有依赖的 WinMD 源文件，供指纹增量检测使用。
    local input_files = {}
    for _, id in ipairs(graph_ctx.order) do
        local spec = graph_ctx.by_id[id]
        table.join2(input_files, spec.winmds)
    end

    depend.on_changed(function()
        os.mkdir(shared.shared_dir)

        -- 按拓扑序计算每个节点的传递依赖 WinMD。
        local dep_refs = {}
        for _, id in ipairs(graph_ctx.order) do
            local spec = graph_ctx.by_id[id]
            local result = {}
            for _, dep_id in ipairs(spec.deps) do
                local dep_spec = graph_ctx.by_id[dep_id]
                if dep_spec then
                    table.join2(result, dep_spec.winmds)
                    table.join2(result, dep_refs[dep_id] or {})
                end
            end
            dep_refs[id] = result
        end

        for _, id in ipairs(graph_ctx.order) do
            local spec = graph_ctx.by_id[id]
            if #spec.winmds > 0 then
                local args = {}
                for _, wm in ipairs(spec.winmds) do
                    table.insert(args, "-in")
                    table.insert(args, wm)
                end
                for _, wm in ipairs(dep_refs[id]) do
                    table.insert(args, "-ref")
                    table.insert(args, wm)
                end
                table.insert(args, "-out")
                table.insert(args, shared.shared_dir)
                os.vrunv(tools.cppwinrt, args)
            end
        end

        return {}
    end, {
        files      = input_files,
        dependfile = path.join(config.builddir(), ".deps", "shared", "shared_projection.d"),
        changed    = option.get("rebuild"),
    })

end
