local context = import("xmake.modules.winui3.context", {rootdir = os.projectdir()})
local util = import("xmake.modules.winui3.utils", {rootdir = os.projectdir()})

function run(target, batchcmds, opt, ctx)
    local missing = context.missing_packages(ctx.packages_dir)
    if #missing == 0 then
        return
    end

    if not ctx.nuget then
        util.fail("nuget.exe not found from environment variables or PATH")
    end

    batchcmds:show_progress(opt.progress, "${color.build.object}restoring.nuget %s", target:name())
    batchcmds:mkdir(ctx.packages_dir)
    batchcmds:vrunv(ctx.nuget, {
        "install",
        ctx.packages_config,
        "-OutputDirectory",
        ctx.packages_dir,
        "-NonInteractive"
    })
end
