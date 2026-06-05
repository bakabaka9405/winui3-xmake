-- NuGet 包统一以全名暴露，规则层直接使用 NuGet 包 ID。
-- 包版本从 packages.config 动态读取，不在本模块中硬编码。
--
-- packages.config 位于项目根目录，是包 ID 与版本的唯一真源。

local _config_versions = nil
local _package_ids = nil
local _nuget_global_dir = nil
local _package_roots = nil

-- 解析 packages.config，返回 NuGet ID 到版本号的映射与有序 ID 列表。
local function _parse_config_versions()
    local config_path = path.join(os.projectdir(), "packages.config")
    if not os.isfile(config_path) then
        raise("packages.config 不存在: " .. config_path)
    end

    local xml = import("core.base.xml")

    local doc = xml.loadfile(config_path)
    if not doc then
        raise("无法解析 packages.config: " .. config_path)
    end

    local root = xml.find(doc, "/packages")
    if not root then
        raise("packages.config 中未找到 <packages> 根元素")
    end

    local versions = {}
    local ids = {}
    for _, pkg in ipairs(root.children or {}) do
        if pkg.name == "package" then
            local pid = pkg.attrs and pkg.attrs.id
            local pver = pkg.attrs and pkg.attrs.version
            if pid and pver then
                versions[pid] = pver
                table.append(ids, pid)
            end
        end
    end

    if #ids == 0 then
        raise("packages.config 中未找到任何 <package> 声明")
    end

    return versions, ids
end

-- NuGet 全局包缓存目录解析。
-- 解析顺序：NUGET_PACKAGES > %USERPROFILE%/.nuget/packages
local function _resolve_nuget_global_dir()
    local env_dir = os.getenv("NUGET_PACKAGES")
    if env_dir and os.isdir(env_dir) then
        return env_dir
    end

    local userprofile = os.getenv("USERPROFILE")
    if userprofile and os.isdir(path.join(userprofile, ".nuget", "packages")) then
        return path.join(userprofile, ".nuget", "packages")
    end

    raise("Cannot resolve NuGet global packages directory.")
end

-- 返回 packages.config 中声明的所有 NuGet 包 ID 列表（按声明顺序）。
-- 数据在模块初始化时一次性构建，直接返回缓存数组。
function all_packages()
    return _package_ids
end

-- 直接从已准备状态解析包根目录。
-- 路径按 NuGet 全局缓存约定定位：
-- <global-dir>/<lowercase-package-id>/<version>
function package_root(nuget_id)
    local version = _config_versions[nuget_id]
    if not version then
        raise("Unknown NuGet package: " .. nuget_id)
    end

    local cache_key = table.concat({nuget_id, tostring(version or "")}, "|")
    local prepared_root = _package_roots[cache_key]
    if prepared_root then
        return prepared_root
    end

    raise("Cannot find package root for " .. nuget_id)
end

-- 解析 packages.config、定位 NuGet 缓存目录、预计算全部包根目录。
do
    local versions, ids = _parse_config_versions()

    -- 定位 NuGet 全局包缓存目录。
    local global_dir = _resolve_nuget_global_dir()

    -- 预先计算所有已声明包的根目录。
    local roots = {}
    for _, nuget_id in ipairs(ids) do
        local ver = versions[nuget_id]
        local pkg_dir = path.join(global_dir, nuget_id:lower(), ver)
        if not os.isdir(pkg_dir) then
            raise(string.format(
                "winui3.packages: NuGet 包目录不存在\n  NuGet ID: %s\n  版本: %s\n  预期路径: %s",
                nuget_id, ver, pkg_dir))
        end
        local cache_key = table.concat({nuget_id, ver}, "|")
        roots[cache_key] = pkg_dir
    end

    _config_versions = versions
    _package_ids = ids
    _nuget_global_dir = global_dir
    _package_roots = roots
end
