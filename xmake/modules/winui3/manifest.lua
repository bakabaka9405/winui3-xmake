-- winui3.manifest：XAML LSP 元数据清单生成模块

local config = import("core.project.config")
local project = import("core.project.project")
local winmd_context = import("winui3.winmd.context")
local json_mod = import("core.base.json")

function export()
    local targets = {}
    for _, target in pairs(project.targets()) do
        if target:is_enabled() then
            local batch = (target:sourcebatches() or {})["winui3.xaml"]
            if batch and batch.sourcefiles and #batch.sourcefiles > 0 then
                table.insert(targets, {target = target, batch = batch})
            end
        end
    end

    table.sort(targets, function (a, b)
        return a.target:fullname() < b.target:fullname()
    end)

    local projects = {}
    for _, item in ipairs(targets) do
        local target = item.target
        local ctx = winmd_context.ensure(target)

        local xaml_files = {}
        for _, file in ipairs(item.batch.sourcefiles) do
            table.insert(xaml_files, path.absolute(file))
        end
        table.sort(xaml_files)

        local ref_assemblies = ctx.ref_winmds
        if #ref_assemblies == 0 then
            ref_assemblies = {}
            json_mod.mark_as_array(ref_assemblies)
        end

        table.insert(projects, {
            id                  = target:fullname(),
            rootNamespace       = target:values("winui3.namespace"),
            windowsSdk          = {
                root    = path.absolute(ctx.sdk_root),
                version = ctx.sdk_version,
            },
            xamlFiles           = xaml_files,
            referenceAssemblies = ref_assemblies,
            localAssembly       = path.absolute(winmd_context.local_assembly_path(target)),
        })
    end

    if #projects == 0 then
        json_mod.mark_as_array(projects)
    end

    local manifest = {
        version       = 1,
        configuration = {
            mode         = config.mode(),
            platform     = config.plat(),
            architecture = config.arch(),
        },
        projects      = projects,
    }

    local encoded = json_mod.encode(manifest, {pretty = true, indent = 2}) .. "\n"
    local output_path = path.join(os.projectdir(), ".xmake", "xaml-lsp.json")
    io.writefile(output_path, encoded)
    return manifest
end
