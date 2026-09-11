-- winui3.winmd.graph：使用 core.base.graph 表达 WinMD 节点之间的依赖关系。
--
-- 职责：
--   1. 声明 WinMD 节点规格（platform / webview2 / appsdk / winuiedit / win2d）。
--   2. 根据应用规则或共享模块 target value 激活根节点，仅收集其依赖闭包中的 WinMD。
--   3. 为每个 target 独立构造活动节点图并调用 topo_sort() 生成拓扑序。
--   4. 基于拓扑序生成 ref_winmds 与 metadata_dirs。
--
-- 边方向约定：依赖项 -> 依赖项的消费者。
--   例如 dag:add_edge("webview2", "appsdk") 表示 appsdk 依赖 webview2。

local graph = import("core.base.graph")

local _target_contexts = {}

local function _collect(node_id)
    return import("winui3.winmd." .. node_id).collect()
end

local NODE_SPECS = {
    {
        id   = "platform",
        deps = {},
    },
    {
        id   = "webview2",
        deps = {"platform"},
    },
    {
        id   = "appsdk",
        rule = "winui3.app",
        deps = {"platform", "webview2"},
    },
    {
        id   = "winuiedit",
        rule = "winuiedit",
        deps = {"appsdk"},
    },
    {
        id   = "win2d",
        rule = "win2d",
        deps = {"appsdk"},
    },
}

function active_root_ids(target)
    local ids = {}
    for _, spec in ipairs(NODE_SPECS) do
        if spec.rule and target:rule(spec.rule) then
            table.insert(ids, spec.id)
        end
    end
    return ids
end

-- 计算激活节点集合：应用目标按自身规则激活；共享模块目标由节点 value 指定根节点。
local function _active_set(target)
    local by_id = {}
    for _, spec in ipairs(NODE_SPECS) do
        by_id[spec.id] = spec
    end

    local active = {}
    local function activate(id)
        if active[id] then
            return
        end
        local spec = by_id[id]
        if not spec then
            raise("winui3.winmd.graph: 未知节点 ID：" .. tostring(id))
        end
        active[id] = true
        for _, dep in ipairs(spec.deps) do
            activate(dep)
        end
    end

    local node_id = target:values("winui3.shared_projection.node")
    if node_id then
        activate(node_id)
    else
        for _, id in ipairs(active_root_ids(target)) do
            activate(id)
        end
    end

    return active
end

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

-- 为目标构造图上下文，仅收集激活节点。
-- 返回 {by_id, order, ref_winmds, metadata_dirs}
function build(target)
    local active = _active_set(target)

    local dag = graph.new(true)
    local by_id = {}

    for _, spec in ipairs(NODE_SPECS) do
        if active[spec.id] then
            local node = table.clone(spec)
            node.winmds = _collect(node.id)
            if #node.winmds == 0 then
                raise("winui3.winmd.graph: 节点 " .. node.id .. " 的 WinMD 列表为空。")
            end
            by_id[node.id] = node
            dag:add_vertex(node.id)
        end
    end

    for _, spec in ipairs(NODE_SPECS) do
        local node = by_id[spec.id]
        if node then
            for _, dep in ipairs(node.deps) do
                if not by_id[dep] then
                    raise("winui3.winmd.graph: 节点 " .. node.id .. " 的依赖 "
                    .. dep .. " 未激活。")
                end
                dag:add_edge(dep, node.id)
            end
        end
    end

    local order, has_cycle = dag:topo_sort()
    if has_cycle then
        raise("winui3.winmd.graph: WinMD 节点依赖存在循环。")
    end

    local ref_winmds = _flatten_order(order, by_id)

    return {
        by_id         = by_id,
        order         = order,
        ref_winmds    = ref_winmds,
        metadata_dirs = _metadata_dirs(ref_winmds),
    }
end

-- 获取或构造目标对应的图上下文。
function ensure(target)
    local key = target:fullname()
    if not _target_contexts[key] then
        _target_contexts[key] = build(target)
    end
    return _target_contexts[key]
end
