-- winui3.winmd.context：WinUI 3 WinMD 构建上下文的统一入口。
--
-- 职责：
--   - import 时立即初始化平台状态（SDK 路径、平台 WinMD）
--   - 调用 winui3.winmd.graph 构造并校验目标级图上下文
--   - 按 target:fullname() 记忆化
--   - 返回供所有下游规则消费的统一上下文表
--
-- 共享投影按 WinMD 节点分目录输出，使 C++ modules 目标可以按依赖图复用，
-- 互不携带无关节点的模块。

local winmd_graph   = import("winui3.winmd.graph")
local platform_mod  = import("winui3.winmd.platform")
local sdk_mod       = import("winui3.sdk")

local _shared_root = path.join(os.projectdir(), "build", ".gens", "shared")
local _target_contexts = {}

local _sdk_root, _sdk_version, _platform_winmds
do
    _sdk_root, _sdk_version = sdk_mod.get_sdk_info()
    _platform_winmds = platform_mod.collect()

    if not _platform_winmds or #_platform_winmds == 0 then
        raise("winui3.winmd.context: 平台 WinMD 列表为空。")
    end
end

--- 某个 WinMD 节点的共享投影生成目录。
function node_dir(node_id)
    return path.join(_shared_root, node_id, "generated")
end

--- 某个 WinMD 节点的共享投影模块目录。
function node_winrt_dir(node_id)
    return path.join(node_dir(node_id), "winrt")
end

--- 为给定 target 构建并缓存 WinMD 上下文。
function ensure(target)
    local cache_key = target:fullname()
    if _target_contexts[cache_key] then
        return _target_contexts[cache_key]
    end

    local graph_ctx = winmd_graph.ensure(target)

    -- 按拓扑序给出节点投影目录，供 includedirs 与模块扫描消费。
    local node_dirs = {}
    for _, node_id in ipairs(graph_ctx.order) do
        table.insert(node_dirs, node_dir(node_id))
    end

    _target_contexts[cache_key] = {
        sdk_root      = _sdk_root,
        sdk_version   = _sdk_version,
        graph         = graph_ctx,
        nodes         = graph_ctx.order,
        node_dirs     = node_dirs,
        ref_winmds    = graph_ctx.ref_winmds,
        metadata_dirs = graph_ctx.metadata_dirs,
    }

    return _target_contexts[cache_key]
end

--- 目标的合并 WinMD 输出路径（本地程序集）。
function local_assembly_path(target)
    local autogen_root = target:autogendir({root = true})
    local namespace = target:values("winui3.namespace") or target:name()
    return path.join(autogen_root, "winmd_merged", namespace .. ".winmd")
end
