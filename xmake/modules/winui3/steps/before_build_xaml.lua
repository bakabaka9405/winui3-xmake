local xaml_inputs = import("xmake.modules.winui3.steps.xaml_inputs", {rootdir = os.projectdir()})

function compile_and_pack(target, batchcmds, opt, ctx)
    batchcmds:show_progress(opt.progress, "${color.build.object}generating.xaml.inputs %s", target:name())
    xaml_inputs.write_xaml_input_json(batchcmds, ctx)

    batchcmds:show_progress(opt.progress, "${color.build.object}compiling.xaml.pass1 %s", target:name())
    batchcmds:vrunv(ctx.xaml_compiler, {ctx.xaml_pass1_in_json, ctx.xaml_pass1_out_json})

    batchcmds:show_progress(opt.progress, "${color.build.object}compiling.xaml.pass2 %s", target:name())
    batchcmds:vrunv(ctx.xaml_compiler, {ctx.xaml_pass2_in_json, ctx.xaml_pass2_out_json})

    batchcmds:show_progress(opt.progress, "${color.build.object}generating.pri %s", target:name())
    xaml_inputs.write_makepri_inputs(batchcmds, ctx)
    batchcmds:vrunv(ctx.makepri, {
        "new",
        "/cf",
        ctx.makepri_config,
        "/pr",
        os.projectdir(),
        "/o",
        "/of",
        ctx.pri_file
    })
end
