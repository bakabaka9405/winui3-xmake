local context = import("xmake.modules.winui3.context", {rootdir = os.projectdir()})
local util = import("xmake.modules.winui3.utils", {rootdir = os.projectdir()})

function generate_base(target, batchcmds, opt, ctx)
    local args_platform = context.make_cppwinrt_args(ctx.generated_dir, ctx.platform_winmds)
    local args_webview2 = context.make_cppwinrt_args(ctx.generated_dir, {ctx.webview2_winmd}, ctx.platform_winmds)

    local appsdk_refs = {}
    for _, p in ipairs(ctx.platform_winmds) do
        table.insert(appsdk_refs, p)
    end
    table.insert(appsdk_refs, ctx.webview2_winmd)
    local args_appsdk = context.make_cppwinrt_args(ctx.generated_dir, ctx.appsdk_winmds, appsdk_refs)

    batchcmds:show_progress(opt.progress, "${color.build.object}generating.cppwinrt.platform %s", target:name())
    batchcmds:mkdir(ctx.generated_dir)
    batchcmds:vrunv(ctx.cppwinrt, args_platform)

    batchcmds:show_progress(opt.progress, "${color.build.object}generating.cppwinrt.webview2 %s", target:name())
    batchcmds:vrunv(ctx.cppwinrt, args_webview2)

    batchcmds:show_progress(opt.progress, "${color.build.object}generating.cppwinrt.appsdk %s", target:name())
    batchcmds:vrunv(ctx.cppwinrt, args_appsdk)
end

function generate_component(target, batchcmds, opt, ctx)
    batchcmds:show_progress(opt.progress, "${color.build.object}generating.cppwinrt.component %s", target:name())
    local args_component = {
        "-out",
        ctx.generated_dir,
        "-comp",
        ctx.generated_dir,
        "-name",
        ctx.root_namespace,
        "-optimize",
        "-prefix",
        "-overwrite",
        "-pch",
        "pch.h",
        "-in",
        ctx.merged_winmd
    }
    util.append_args(args_component, "-ref", ctx.winmd_refs)
    batchcmds:vrunv(ctx.cppwinrt, args_component)
end
