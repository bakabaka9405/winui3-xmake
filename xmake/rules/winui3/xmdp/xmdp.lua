local utils = import("utils")

function after_load(target)
    target:add("files", path.join(target:autogendir({root = true}), "generated", "XamlMetaDataProvider.cpp"), {always_added = true})
end

function on_config(target)
    local namespace = target:values("winui3.namespace")
    local generated_dir = path.join(target:autogendir({root = true}), "generated")
    local idl_path = path.join(generated_dir, "XamlMetaDataProvider.idl")
    local cpp_path = path.join(generated_dir, "XamlMetaDataProvider.cpp")

    local idl_content = string.format(
        "namespace %s\n{\n"
        .. "    runtimeclass XamlMetaDataProvider : [default] Microsoft.UI.Xaml.Markup.IXamlMetadataProvider\n"
        .. "    {\n"
        .. "        XamlMetaDataProvider();\n"
        .. "    }\n"
        .. "}\n",
        namespace
    )

    local cpp_content = '#include "pch.h"\n\n'
        .. '#include "XamlMetaDataProvider.h"\n'
        .. '#include "XamlMetaDataProvider.g.cpp"\n'

    utils.write_file_if_changed(idl_path, idl_content)
    utils.write_file_if_changed(cpp_path, cpp_content)
end

