-- winui3.winmd.graph：使用 core.base.graph 表达 WinMD 节点之间的依赖关系。
--
-- 职责：
--   1. 声明 WinMD 节点规格（platform / webview2 / appsdk / win2d）。
--   2. 收集所有 WinMD 节点。
--   3. 构造有向图并调用 topo_sort() 生成拓扑序。
--   4. 基于拓扑序生成 ref_winmds 与 metadata_dirs。
--
-- 边方向约定：依赖项 -> 依赖项的消费者。
--   例如 dag:add_edge("webview2", "appsdk") 表示 appsdk 依赖 webview2。

local graph = import("core.base.graph")

local _target_contexts = {}

local NODE_SPECS = {
    {
        id      = "platform",
        deps    = {},
        collect = function(target)
            return import("winui3.winmd.platform").collect()
        end,
    },
    {
        id      = "webview2",
        deps    = {"platform"},
        collect = function(target)
            return import("winui3.winmd.webview2").collect()
        end,
    },
    {
        id      = "appsdk",
        deps    = {"platform", "webview2"},
        collect = function(target)
            return import("winui3.winmd.appsdk").collect()
        end,
    },
    {
        id      = "win2d",
        deps    = {"platform", "webview2", "appsdk"},
        collect = function(target)
            return import("winui3.winmd.win2d").collect()
        end,
    },
}

-- 按拓扑序展平 WinMD 列表。
local function _flatten_order(order, by_id)
    local result = {}
    for _, id in ipairs(order) do
        local spec = by_id[id]
        table.join2(result, spec.winmds)
    end
    return result
end

-- 从 WinMD 列表提取去重元数据目录列表，供 mdmerge -metadata_dir 使用。
local function _metadata_dirs(ref_winmds)
    local result = {}
    local seen = {}
    for _, wm in ipairs(ref_winmds) do
        local dir = path.directory(wm)
        if not seen[dir] then
            seen[dir] = true
            table.insert(result, dir)
        end
    end
    return result
end

-- 为目标构造完整的图上下文。
-- 返回 {by_id, order, ref_winmds, metadata_dirs}
function build(target)
    local dag = graph.new(true)
    local by_id = {}

    -- 收集所有节点并注册顶点。
    for _, spec in ipairs(NODE_SPECS) do
        local active = table.clone(spec)
        active.winmds = active.collect(target)
        by_id[active.id] = active
        dag:add_vertex(active.id)
    end

    -- 注册依赖边。
    for id, spec in pairs(by_id) do
        for _, dep in ipairs(spec.deps) do
            dag:add_edge(dep, id)
        end
    end

    local order = dag:topo_sort()

    local ref_winmds = _flatten_order(order, by_id)

    return {
        by_id        = by_id,
        order        = order,
        ref_winmds   = ref_winmds,
        metadata_dirs = _metadata_dirs(ref_winmds),
    }
end

-- 获取或构造目标对应的图上下文（按 target:fullname() 缓存）。
function ensure(target)
    local key = target:fullname()
    if not _target_contexts[key] then
        _target_contexts[key] = build(target)
    end
    return _target_contexts[key]
end

-- 从图上下文中按节点 ID 获取 WinMD 列表。
function get_winmds(ctx, id)
    local spec = ctx.by_id[id]
    return spec and spec.winmds or {}
end
