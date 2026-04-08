local on_load = import("xmake.modules.winui3.steps.on_load", {rootdir = os.projectdir()})
local before_build = import("xmake.modules.winui3.steps.before_build", {rootdir = os.projectdir()})
local after_build = import("xmake.modules.winui3.steps.after_build", {rootdir = os.projectdir()})

function apply_on_load(target, ctx)
    on_load.apply(target, ctx)
end

function run_before_buildcmd(target, batchcmds, opt, ctx)
    before_build.run(target, batchcmds, opt, ctx)
end

function run_after_buildcmd(target, batchcmds, ctx)
    after_build.run(target, batchcmds, ctx)
end
