local restore = import("xmake.modules.winui3.steps.before_build_restore", {rootdir = os.projectdir()})
local cppwinrt = import("xmake.modules.winui3.steps.before_build_cppwinrt", {rootdir = os.projectdir()})
local winmd = import("xmake.modules.winui3.steps.before_build_winmd", {rootdir = os.projectdir()})
local xaml = import("xmake.modules.winui3.steps.before_build_xaml", {rootdir = os.projectdir()})

function run(target, batchcmds, opt, ctx)
    restore.run(target, batchcmds, opt, ctx)
    cppwinrt.generate_base(target, batchcmds, opt, ctx)
    winmd.generate_and_merge(target, batchcmds, opt, ctx)
    cppwinrt.generate_component(target, batchcmds, opt, ctx)
    xaml.compile_and_pack(target, batchcmds, opt, ctx)
end
