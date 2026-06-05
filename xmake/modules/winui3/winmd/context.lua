-- winui3.winmd.context：WinUI 3 WinMD 构建上下文的统一入口。
--
-- 职责：
--   - import 时立即初始化平台状态（SDK 路径、平台 WinMD）
--   - 调用 winui3.winmd.graph 构造目标级图上下文
--   - 校验 AppSDK / WebView2 WinMD 非空
--   - 按 target:fullname() 记忆化，每个目标仅计算一次
--   - 返回供所有下游规则消费的统一上下文表

local winmd_graph   = import("winui3.winmd.graph")
local platform_mod  = import("winui3.winmd.platform")
local sdk_mod       = import("winui3.sdk")

local _shared_dir = path.join(os.projectdir(), "build", ".gens", "shared", "generated")
local _target_contexts = {}

local _sdk_root, _sdk_version, _platform_winmds
do
    _sdk_root, _sdk_version = sdk_mod.get_sdk_info()
    _platform_winmds = platform_mod.collect()

    if not _platform_winmds or #_platform_winmds == 0 then
        raise("winui3.winmd.context: 平台 WinMD 列表为空。")
    end
end

--- 为给定 target 构建并缓存 WinMD 上下文。
--- 返回的表包含所有下游规则所需字段，后续调用直接返回缓存值。
function ensure(target)
    local target_key = target:fullname()
    if _target_contexts[target_key] then
        return _target_contexts[target_key]
    end

    local graph_ctx = winmd_graph.ensure(target)

    if #winmd_graph.get_winmds(graph_ctx, "appsdk") == 0 then
        raise("winui3.winmd.context: WinAppSDK WinMD 列表为空。")
    end
    if #winmd_graph.get_winmds(graph_ctx, "webview2") == 0 then
        raise("winui3.winmd.context: WebView2 WinMD 未收集。")
    end

    _target_contexts[target_key] = {
        shared_dir    = _shared_dir,
        sdk_root      = _sdk_root,
        sdk_version   = _sdk_version,
        graph         = graph_ctx,
        ref_winmds    = graph_ctx.ref_winmds,
        metadata_dirs = graph_ctx.metadata_dirs,
    }

    return _target_contexts[target_key]
end
