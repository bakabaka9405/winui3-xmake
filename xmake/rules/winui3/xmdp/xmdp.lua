-- 仅在内容变化时写入，保持未变文件的 mtime，避免增量构建重复触发。
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

function after_load(target)
    target:add("files", path.join(target:autogendir({root = true}), "generated", "XamlMetaDataProvider.cpp"), {always_added = true})
end

function on_prepare(target)
    local namespace = target:values("winui3.namespace")
    local generated_dir = path.join(target:autogendir({root = true}), "generated")
    local idl_path = path.join(generated_dir, "XamlMetaDataProvider.idl")
    local cpp_path = path.join(generated_dir, "XamlMetaDataProvider.cpp")

    os.mkdir(generated_dir)

    local idl_content = string.format(
        "namespace %s\n{\n"
        .. "    runtimeclass XamlMetaDataProvider : [default] Microsoft.UI.Xaml.Markup.IXamlMetadataProvider\n"
        .. "    {\n"
        .. "        XamlMetaDataProvider();\n"
        .. "    }\n"
        .. "}\n",
        namespace
    )

    local cpp_content = '#include "pch.h"\n'
        .. '#include "XamlMetaDataProvider.h"\n'
        .. '#include "XamlMetaDataProvider.g.cpp"\n'

    write_file_if_changed(idl_path, idl_content)
    write_file_if_changed(cpp_path, cpp_content)
end

