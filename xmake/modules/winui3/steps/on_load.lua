function apply(target, ctx)
    local system_include_dirs = {
        ctx.xaml_generated_dir,
        ctx.xaml_generated_src_dir,
        ctx.generated_dir,
        path.join(ctx.include_dir, "cppwinrt"),
        ctx.implementation_include,
        ctx.appsdk_include,
        ctx.project_dir,
        ctx.source_dir
    }
    for _, dir in ipairs(system_include_dirs) do
        target:add("sysincludedirs", dir)
    end
    target:add("linkdirs", ctx.appsdk_libdir)
    target:add("links", "Microsoft.WindowsAppRuntime.Bootstrap")
end
