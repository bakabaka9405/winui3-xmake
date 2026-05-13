task("gen-xmdp")
    set_category("plugin")
    on_run(function ()
        local task = import("core.project.task")
        local option = import("core.base.option")
        local project = import("core.project.project")

        task.run("config")
        local target = project.target(option.get("target"))
        local gen_dir = path.join(target:autogendir({root = true}), "generated")
        local namespace = target:extraconf("rules", "winui3.app", "namespace")
        local xmp_idl = path.join(gen_dir, "XamlMetaDataProvider.idl")
        local xmp_file = path.join(gen_dir, "XamlMetaDataProvider.cpp")
        if not os.isdir(gen_dir) then
            os.mkdir(gen_dir)
        end

        local function write_file_if_changed(filepath, content)
            if os.isfile(filepath) then
                local existing = io.readfile(filepath)
                if existing == content then
                    return false
                end
            end
            io.writefile(filepath, content)
            return true
        end

        write_file_if_changed(xmp_idl, string.format(
            "namespace %s\n{\n    runtimeclass XamlMetaDataProvider : [default] Microsoft.UI.Xaml.Markup.IXamlMetadataProvider\n    {\n        XamlMetaDataProvider();\n    }\n}\n",
            namespace
        ))
        write_file_if_changed(xmp_file, '#include "pch.h"\n#include "XamlMetaDataProvider.h"\n#include "XamlMetaDataProvider.g.cpp"\n')
        target:add("files", xmp_file)
    end)

    set_menu {
        usage   = "xmake gen-xmdp [target]",
        description = "Generate XamlMetaDataProvider implementation for a WinUI target.",
        options = {
            {nil, "target", "v", nil, "Target name"}
        }
    }
