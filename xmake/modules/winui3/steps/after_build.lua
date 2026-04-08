function run(target, batchcmds, ctx)
    batchcmds:cp(ctx.bootstrap_dll, target:targetdir())
    batchcmds:cp(ctx.pri_file, path.join(target:targetdir(), "test.pri"))
end
