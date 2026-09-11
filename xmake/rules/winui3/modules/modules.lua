-- winui3.modules 规则实现

function on_load(target)
    target:set("policy", "build.c++.modules", true)

    -- 每个 rule 直接激活的根节点依赖对应共享模块目标，传递依赖由节点 target 图处理。
    local winmd_graph = import("winui3.winmd.graph")
    for _, node_id in ipairs(winmd_graph.active_root_ids(target)) do
        target:add("deps", "winui3.shared_projection.modules." .. node_id)
    end
end

function on_config(target)
    local namespace = target:values("winui3.namespace")
    target:add("defines", "WINRT_ENABLE_LEGACY_COM", "WINUI3_IMPORT_MODULE=" .. namespace .. ".winrt")
end
