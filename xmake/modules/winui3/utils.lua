function fail(message)
    os.raise(message)
end

function split_version(version)
    local values = {}
    for n in (version or ""):gmatch("%d+") do
        table.insert(values, tonumber(n))
    end
    return values
end

function compare_version(left, right)
    local lparts = split_version(left)
    local rparts = split_version(right)
    local total = math.max(#lparts, #rparts)
    for i = 1, total do
        local lv = lparts[i] or 0
        local rv = rparts[i] or 0
        if lv < rv then
            return -1
        end
        if lv > rv then
            return 1
        end
    end
    return 0
end

function latest_dir(pattern, prefix)
    local dirs = os.dirs(pattern)
    if #dirs == 0 then
        return nil
    end
    table.sort(dirs, function(left, right)
        local lname = path.filename(left)
        local rname = path.filename(right)
        if prefix then
            lname = lname:gsub("^" .. prefix, "")
            rname = rname:gsub("^" .. prefix, "")
        end
        return compare_version(lname, rname) > 0
    end)
    return dirs[1]
end

function split_env_paths(paths)
    local result = {}
    if not paths then
        return result
    end
    for item in (paths .. ";"):gmatch("([^;]*);") do
        if item ~= "" then
            table.insert(result, path.translate(item))
        end
    end
    return result
end

function find_nuget_from_env()
    local path_env = os.getenv("PATH") or os.getenv("Path")
    for _, folder in ipairs(split_env_paths(path_env)) do
        local exe = path.join(folder, "nuget.exe")
        if os.isfile(exe) then
            return exe
        end
    end
    return nil
end

local function _raise(fail_fn, message)
    local raise_fn = fail_fn or fail
    raise_fn(message)
end

function package_path(packages_dir, package_id, package_versions, fail_fn)
    local version = package_versions[package_id]
    if not version then
        _raise(fail_fn, "missing package version for " .. package_id)
    end
    return path.join(packages_dir, package_id .. "." .. version)
end

function sdk_root(fail_fn)
    local from_env = os.getenv("WindowsSdkDir")
    if from_env and os.isdir(from_env) then
        return path.translate(from_env)
    end

    local root = "C:/Program Files (x86)/Windows Kits/10"
    if os.isdir(root) then
        return root
    end
    _raise(fail_fn, "Windows 10 SDK root not found")
end

function platform_winmds(sdk_root_dir, sdk_version)
    return os.files(path.join(sdk_root_dir, "References", sdk_version, "*", "*", "*.winmd"))
end

function pick_first_existing_or_default(paths)
    for _, p in ipairs(paths) do
        if os.isfile(p) then
            return p
        end
    end
    return paths[1]
end

function appsdk_winmd_path(appsdk_lib, uap_dirs, name)
    local candidates = {}
    for _, dir_name in ipairs(uap_dirs) do
        table.insert(candidates, path.join(appsdk_lib, dir_name, name .. ".winmd"))
    end
    return pick_first_existing_or_default(candidates)
end

function append_args(args, flag, values)
    for _, p in ipairs(values or {}) do
        table.insert(args, flag)
        table.insert(args, p)
    end
end

function filter_core_platform_winmds(platform_refs)
    local core = {}
    for _, p in ipairs(platform_refs or {}) do
        local name = path.filename(p)
        if name == "Windows.Foundation.FoundationContract.winmd" or name == "Windows.Foundation.UniversalApiContract.winmd" then
            table.insert(core, p)
        end
    end
    if #core == 0 and #platform_refs > 0 then
        table.insert(core, platform_refs[1])
    end
    return core
end

function append_unique(list, value)
    for _, item in ipairs(list) do
        if item == value then
            return
        end
    end
    table.insert(list, value)
end

function find_foundation_metadata_dir(sdk_root_dir, sdk_version, fail_fn)
    local refs_root = path.join(sdk_root_dir, "References", sdk_version)
    local candidates = {
        path.join(refs_root, "Windows.Foundation.FoundationContract", "*"),
        path.join(refs_root, "windows.foundation.foundationcontract", "*")
    }

    for _, pattern in ipairs(candidates) do
        local dirs = os.dirs(pattern)
        if #dirs > 0 then
            table.sort(dirs, function(left, right)
                return compare_version(path.filename(left), path.filename(right)) > 0
            end)
            return dirs[1]
        end
    end
    _raise(fail_fn, "failed to find Windows.Foundation.FoundationContract metadata directory")
end

function sdk_buildtools_bindir(packages_dir, package_versions, fail_fn)
    local sdk_tools = package_path(packages_dir, "Microsoft.Windows.SDK.BuildTools", package_versions, fail_fn)
    local version_dir = latest_dir(path.join(sdk_tools, "bin", "*"))
    if not version_dir then
        _raise(fail_fn, "failed to find SDK BuildTools bin directory")
    end
    local x64_dir = path.join(version_dir, "x64")
    if not os.isdir(x64_dir) then
        _raise(fail_fn, "failed to find x64 SDK BuildTools bin directory")
    end
    return x64_dir
end
