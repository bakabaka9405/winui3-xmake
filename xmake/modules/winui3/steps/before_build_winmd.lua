local util = import("xmake.modules.winui3.utils", {rootdir = os.projectdir()})

function generate_and_merge(target, batchcmds, opt, ctx)
    batchcmds:show_progress(opt.progress, "${color.build.object}generating.winmd.mainwindow %s", target:name())
    os.mkdir(ctx.winmd_unmerged_dir)

    local cpp_cmd = target:tool("cxx") or "cl.exe"
    local cpp_dir = path.directory(cpp_cmd)
    local midl_envs = {PATH = cpp_dir .. ";" .. (os.getenv("PATH") or "")}
    local midl_include_dirs = {
        path.join(ctx.include_dir, "winrt"),
        path.join(ctx.include_dir, "shared"),
        path.join(ctx.include_dir, "um"),
        path.join(ctx.include_dir, "ucrt")
    }

    local mainwindow_midl_args = {
        ctx.mainwindow_idl,
        "/nologo",
        "/winrt",
        "/winmd",
        ctx.mainwindow_winmd,
        "/nomidl",
        "/h",
        "nul",
        "/metadata_dir",
        ctx.foundation_metadata_dir
    }
    util.append_args(mainwindow_midl_args, "/I", midl_include_dirs)
    util.append_args(mainwindow_midl_args, "/reference", ctx.winmd_refs)
    batchcmds:vrunv(ctx.midl, mainwindow_midl_args, {envs = midl_envs})

    batchcmds:show_progress(opt.progress, "${color.build.object}generating.winmd.xamlmetadata %s", target:name())
    local xamlmeta_midl_args = {
        ctx.xamlmetadata_idl,
        "/nologo",
        "/winrt",
        "/winmd",
        ctx.xamlmetadata_winmd,
        "/nomidl",
        "/h",
        "nul",
        "/metadata_dir",
        ctx.foundation_metadata_dir
    }
    util.append_args(xamlmeta_midl_args, "/I", midl_include_dirs)
    util.append_args(xamlmeta_midl_args, "/reference", ctx.winmd_refs)
    batchcmds:vrunv(ctx.midl, xamlmeta_midl_args, {envs = midl_envs})

    batchcmds:show_progress(opt.progress, "${color.build.object}merging.winmd %s", target:name())
    os.mkdir(ctx.winmd_merged_dir)
    local mdmerge_args = {"-o", ctx.winmd_merged_dir}
    local metadata_dirs = {}
    for _, ref_path in ipairs(ctx.winmd_refs) do
        util.append_unique(metadata_dirs, path.directory(ref_path))
    end
    util.append_args(mdmerge_args, "-metadata_dir", metadata_dirs)
    table.insert(mdmerge_args, "-i")
    table.insert(mdmerge_args, ctx.mainwindow_winmd)
    table.insert(mdmerge_args, "-i")
    table.insert(mdmerge_args, ctx.xamlmetadata_winmd)
    table.insert(mdmerge_args, "-partial")
    table.insert(mdmerge_args, "-n:1")
    batchcmds:vrunv(ctx.mdmerge, mdmerge_args)
end
