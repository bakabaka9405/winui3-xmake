local context = import("xmake.modules.winui3.context", {rootdir = os.projectdir()})
local steps = import("xmake.modules.winui3.steps", {rootdir = os.projectdir()})
local util = import("xmake.modules.winui3.utils", {rootdir = os.projectdir()})
local CTX_KEY = "winui3_ctx"

function on_load(target)
    if not is_plat("windows") then
        util.fail("only supports Windows")
    end

    local target_ctx = context.target_context()
    target:data_set(CTX_KEY, target_ctx)
    steps.apply_on_load(target, target_ctx)
end

function before_buildcmd(target, batchcmds, opt)
    local target_ctx = target:data(CTX_KEY)
    if not target_ctx then
        util.fail("missing WinUI3 context")
    end

    steps.run_before_buildcmd(target, batchcmds, opt, target_ctx)
end

function after_buildcmd(target, batchcmds)
    local target_ctx = target:data(CTX_KEY)
    if target_ctx then
        steps.run_after_buildcmd(target, batchcmds, target_ctx)
    end
end
