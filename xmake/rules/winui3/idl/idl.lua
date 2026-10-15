local utils = import("utils")
local depend = import("core.project.depend")
local winmd_context = import("winui3.winmd.context")
local option = import("core.base.option")


-- 保留 IDL 相对目录结构，避免同名 IDL 在未合并 WinMD 目录中互相覆盖。
local function _idl_to_winmd(out_dir, idl_path, base_dir)
    local idl_stem = path.basename(idl_path)
    if base_dir and #base_dir > 0 then
        local base_normalized = path.translate(base_dir:gsub("[/\\]+$", ""))
        local idl_normalized = path.translate(idl_path)
        if idl_normalized:sub(1, #base_normalized) == base_normalized then
            local rel = idl_normalized:sub(#base_normalized + 2)
            local rel_dir = path.directory(rel)
            if rel_dir and #rel_dir > 0 then
                return path.join(out_dir, rel_dir, idl_stem .. ".winmd")
            end
        end
    end
    return path.join(out_dir, idl_stem .. ".winmd")
end

local function _append_unique(result, seen, value)
    if value and not seen[value] then
        seen[value] = true
        table.insert(result, value)
    end
end

-- 仅枚举目标实际激活的 WinMD 节点投影模块，避免导入无关模块。
local function _shared_module_names(shared)
    local result = {}
    local seen = {}

    _append_unique(result, seen, "winrt_base")
    _append_unique(result, seen, "winrt_numerics")

    for _, node_id in ipairs(shared.nodes) do
        local module_files = os.files(path.join(
            winmd_context.node_winrt_dir(node_id), "*.ixx"))
        table.sort(module_files)
        for _, module_file in ipairs(module_files) do
            _append_unique(result, seen, path.basename(module_file))
        end
    end

    if #result == 2 then
        raise("winui3.idl: shared projection modules were not generated before aggregate module creation.")
    end

    return result
end

local function _write_aggregate_module(module_file, import_name, namespace, shared)
    local lines = {"export module " .. import_name .. ";", ""}

    table.insert(lines, "export import std;")
    table.insert(lines, "")

    for _, module_name in ipairs(_shared_module_names(shared)) do
        table.insert(lines, "export import " .. module_name .. ";")
    end

    table.insert(lines, "")
    table.insert(lines, "export import winrt." .. namespace .. ";")
    table.insert(lines, "")

    os.mkdir(path.directory(module_file))
    utils.write_file_if_changed(module_file, table.concat(lines, "\n"))
end

function after_load(target)
    local generated_dir = path.join(target:autogendir({root = true}), "generated")

    target:add("files", path.join(generated_dir, "XamlMetaDataProvider.idl"), {always_added = true})
    target:add("includedirs", path.join(generated_dir, "winrt"))
    if target:rule("winui3.modules") then
        local namespace = target:values("winui3.namespace")
        target:add("files", path.join(generated_dir, "winrt", "winrt." .. namespace .. ".ixx"), {always_added = true})
        target:add("files", path.join(generated_dir, namespace .. ".winrt.cppm"), {always_added = true})
    end
end

function before_prepare_files(target, sourcebatch, opt)
    local idl_files = sourcebatch.sourcefiles
    local shared = winmd_context.ensure(target)

    local namespace = target:values("winui3.namespace")
    local is_modules = target:rule("winui3.modules") ~= nil
    local import_name = is_modules and (namespace .. ".winrt") or nil
    local autogen_root  = target:autogendir({root = true})
    local generated_dir = path.join(autogen_root, "generated")
    local unmerged_dir = path.join(autogen_root, "winmd_unmerged")
    local merged_winmd = winmd_context.local_assembly_path(target)
    local merged_dir = path.directory(merged_winmd)
    local aggregate_module_file = is_modules and path.join(generated_dir, namespace .. ".winrt.cppm") or nil

    local dependfile = path.join(target:dependir({root = true}), "idl.d")
    local compiler = target:compiler("cxx")

    depend.on_changed(function()
        local tools = import("winui3.tools")()
        local midl_exe    = tools.midl
        local mdmerge_exe = tools.mdmerge
        local cppwinrt_exe = tools.cppwinrt

        local metadata_dir = import("winui3.winmd.platform").find_metadata_dir()

        local sdk_root = shared.sdk_root
        local sdk_version = shared.sdk_version
        if not sdk_root or not sdk_version then
            raise("winui3.idl: cannot determine Windows SDK root/version."
                .. "\n  SDK root: " .. tostring(sdk_root)
                .. "\n  SDK version: " .. tostring(sdk_version)
                .. "\n  Ensure Visual Studio 2022 with Windows SDK is installed.")
        end
        local sdk_include_dir = path.join(sdk_root, "Include", sdk_version)
        local sdk_include_um     = path.join(sdk_include_dir, "um")
        local sdk_include_shared = path.join(sdk_include_dir, "shared")
        local sdk_include_winrt  = path.join(sdk_include_dir, "winrt")

        local ref_winmds = shared.ref_winmds

        for _, idl_path in ipairs(idl_files) do
            local out_winmd = _idl_to_winmd(unmerged_dir, idl_path, os.projectdir())
            local out_parent = path.directory(out_winmd)
            os.mkdir(out_parent)

            local midl_args = {
                "/nologo",
                "/winrt",
                "/winmd", path.absolute(out_winmd),
                "/nomidl",
                "/h", "nul",
                "/metadata_dir", metadata_dir,
                "/I", sdk_include_um,
                "/I", sdk_include_shared,
                "/I", sdk_include_winrt,
                "/I", path.directory(idl_path),
            }
            local prefixargs = {idl_path, "/cpp_cmd", path.filename(compiler:program())}
            if target:has_tool("cxx", "clang") then
                -- cpp_opt 覆盖预处理参数，需显式传递 SDK 与当前 IDL 的头文件目录。
                table.join2(prefixargs, {"/cpp_opt", os.args({
                    "-E", "-x", "c",
                    "-I", sdk_include_um,
                    "-I", sdk_include_shared,
                    "-I", sdk_include_winrt,
                    "-I", path.directory(idl_path),
                })})
            end
            for _, ref in ipairs(ref_winmds) do
                table.insert(midl_args, "/reference")
                table.insert(midl_args, path.absolute(ref))
            end

            os.vrunv(midl_exe, table.join(prefixargs, winos.cmdargv(midl_args)), {envs = compiler:runenvs()})
        end

        local unmerged_winmds = os.files(path.join(unmerged_dir, "**.winmd"))
        if #unmerged_winmds == 0 then
            raise("winui3.idl: no .winmd files found in " .. unmerged_dir
                .. " — MIDL compilation may have failed.")
        end

        local ref_dirs = shared.metadata_dirs

        local mdmerge_args = {"-o", merged_dir}
        for _, dir in ipairs(ref_dirs) do
            table.insert(mdmerge_args, "-metadata_dir")
            table.insert(mdmerge_args, dir)
        end
        for _, wm in ipairs(unmerged_winmds) do
            table.insert(mdmerge_args, "-i")
            table.insert(mdmerge_args, wm)
        end
        table.insert(mdmerge_args, "-partial")
        table.insert(mdmerge_args, "-n:1")

        os.vrunv(mdmerge_exe, winos.cmdargv(mdmerge_args))

        if not os.isfile(merged_winmd) then
            raise("winui3.idl: merged WinMD not found: " .. merged_winmd)
        end

        local cppwinrt_args = {
            "-in",  merged_winmd,
            "-out", generated_dir,
            "-comp",
            "-name", namespace,
            "-pch", ".",
            "-prefix",
            "-optimize",
            "-overwrite",
        }
        if import_name then
            table.insert(cppwinrt_args, "-modules")
        end
        for _, ref in ipairs(ref_winmds) do
            table.insert(cppwinrt_args, "-ref")
            table.insert(cppwinrt_args, ref)
        end

        os.vrunv(cppwinrt_exe, winos.cmdargv(cppwinrt_args))

        if import_name then
            _write_aggregate_module(aggregate_module_file, import_name, namespace, shared)
        end

    end, {
        files      = idl_files,
        values     = {compiler:program()},
        dependfile = dependfile,
        changed    = option.get("rebuild")
    })
end
