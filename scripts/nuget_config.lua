-- shared packages.config parser for WinUI3 / Win2D xmake rules
--
-- Provides two interfaces that lazily parse packages.config on first call
-- and cache the result at module level:
--
--   all_packages()   → paths: table (package id → local path),
--                       package_ids: table (ordered list, preserved
--                       declaration order from packages.config)
--   package_path(id) → local path string, or nil if not found
--
-- Important: import("core.base.xml") is called inside the parser function.
-- Do NOT move it to module top-level — xmake's import() requires a
-- function scope.

local _cached = nil

-- ── lazy parser ────────────────────────────────────────

local function _parse()
    if _cached then
        return _cached
    end

    -- validate environment
    local userprofile = os.getenv("USERPROFILE")
    if not userprofile then
        raise("USERPROFILE environment variable is not set; cannot resolve NuGet global packages directory")
    end
    local config_path = path.join(os.projectdir(), "packages.config")
    if not os.isfile(config_path) then
        raise("packages.config not found at " .. config_path)
    end
    local nuget_root = userprofile:gsub("\\", "/") .. "/.nuget/packages"

    local xml = import("core.base.xml")
    local doc = xml.loadfile(config_path)
    local root = xml.find(doc, "/packages")

    local paths = {}
    local package_ids = {}

    if root then
        for _, pkg in ipairs(root.children or {}) do
            if pkg.name == "package" then
                local pid = pkg.attrs and pkg.attrs.id
                local pver = pkg.attrs and pkg.attrs.version
                if pid and pver then
                    paths[pid] = nuget_root .. "/" .. string.lower(pid) .. "/" .. pver
                    table.insert(package_ids, pid)
                end
            end
        end
    end

    _cached = { paths = paths, package_ids = package_ids }
    return _cached
end

-- ── public API ─────────────────────────────────────────

function all_packages()
    local c = _parse()
    return c.paths, c.package_ids
end

function package_path(id)
    local c = _parse()
    return c.paths[id]
end

return {
    all_packages = all_packages,
    package_path = package_path,
}
