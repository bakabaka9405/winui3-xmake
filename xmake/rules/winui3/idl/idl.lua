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

function after_load(target)
    target:add("files", path.join(target:autogendir({root = true}), "generated", "XamlMetaDataProvider.idl"), {always_added = true})
    target:add("includedirs", path.join(target:autogendir({root = true}), "generated", "winrt"))
end

-- 增量指纹跟踪 IDL 源、规则模块以及包/SDK 版本；输出必须包含 merged_winmd 与投影目录。
function before_build_files(target, jobgraph, sourcebatch, opt)
    local shared = winmd_context.ensure(target)

    local namespace = target:values("winui3.namespace")
    local autogen_root  = target:autogendir({root = true})
    local generated_dir = path.join(autogen_root, "generated")
    local unmerged_dir = path.join(autogen_root, "winmd_unmerged")
    local merged_dir = path.join(autogen_root, "winmd_merged")
    local merged_winmd = path.join(autogen_root, "winmd_merged", namespace .. ".winmd")
    local projection_dir = path.join(generated_dir, "winrt")

    local idl_batch = target:sourcebatches()["winui3.idl"]
    local idl_files = idl_batch.sourcefiles

    local dependfile = path.join(target:dependir({root = true}), "idl.d")

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

        os.mkdir(unmerged_dir)
        os.mkdir(merged_dir)
        os.mkdir(generated_dir)

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
            for _, ref in ipairs(ref_winmds) do
                table.insert(midl_args, "/reference")
                table.insert(midl_args, path.absolute(ref))
            end

            -- 引用 WinMD 较多时使用响应文件，避免命令行长度限制截断参数。
            local all_args = table.join({idl_path}, midl_args)
            if #all_args > 80 then
                local rsp = utils.write_response_file(
                    target, path.basename(idl_path) .. ".rsp", midl_args)
                os.vrunv(midl_exe, {idl_path, "@" .. rsp}, {envs = target:toolchain("msvc"):runenvs()})
            else
                os.vrunv(midl_exe, all_args, {envs = target:toolchain("msvc"):runenvs()})
            end
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

        -- mdmerge 与 cppwinrt 均可能携带大量引用路径，沿用响应文件策略。
        if #mdmerge_args > 80 then
            local rsp = utils.write_response_file(target, "_mdmerge.rsp", mdmerge_args)
            os.vrunv(mdmerge_exe, {"@" .. rsp}, {envs = target:toolchain("msvc"):runenvs()})
        else
            os.vrunv(mdmerge_exe, mdmerge_args, {envs = target:toolchain("msvc"):runenvs()})
        end

        if not os.isfile(merged_winmd) then
            raise("winui3.idl: merged WinMD not found: " .. merged_winmd)
        end

        local cppwinrt_args = {
            "-in",  merged_winmd,
            "-out", generated_dir,
            "-comp",
            "-name", namespace,
            "-pch", "pch.h",
            "-prefix",
            "-optimize",
            "-overwrite",
        }
        for _, ref in ipairs(ref_winmds) do
            table.insert(cppwinrt_args, "-ref")
            table.insert(cppwinrt_args, ref)
        end

        if #cppwinrt_args > 80 then
            local rsp = utils.write_response_file(target, "_cppwinrt.rsp", cppwinrt_args)
            os.vrunv(cppwinrt_exe, {"@" .. rsp}, {envs = target:toolchain("msvc"):runenvs()})
        else
            os.vrunv(cppwinrt_exe, cppwinrt_args, {envs = target:toolchain("msvc"):runenvs()})
        end

    end, {
        files      = idl_files,
        dependfile = dependfile,
        changed    = option.get("rebuild"),
    })

end