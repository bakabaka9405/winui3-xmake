-- Windows SDK 自动发现模块。
--
-- import 时自动按 WindowsSdkDir 环境变量与默认安装目录查找 SDK，
-- 缓存 SDK 根路径与版本。供 winui3.winmd.platform 和下游规则消费。
--
-- 导出：
--   get_sdk_info() -> sdk_root, sdk_version

local utils = import("utils")

local _sdk_root = nil
local _sdk_version = nil

--- 扫描 SDK UAP 平台目录，返回最新版本号。
function discover_uap_version(sdk_root)
    local uap_dir = path.join(sdk_root, "Platforms", "UAP")
    if not os.isdir(uap_dir) then
        return nil
    end

    local names = {}
    for _, entry in ipairs(os.dirs(path.join(uap_dir, "*"))) do
        names[#names + 1] = path.filename(entry)
    end
    return utils.max_version(names)
end

local function _discover_sdk()
    local env_sdk = os.getenv("WindowsSdkDir")
    if env_sdk and os.isdir(env_sdk) then
        local root = path.translate(env_sdk:gsub("[/\\]+$", ""))
        local ver = discover_uap_version(root)
        if ver then
            return root, ver
        end
    end

    local default_sdk = "C:\\Program Files (x86)\\Windows Kits\\10"
    if os.isdir(default_sdk) then
        local ver = discover_uap_version(default_sdk)
        if ver then
            return default_sdk, ver
        end
    end

    raise("winui3.sdk: Cannot locate Windows SDK.")
end

--- 返回缓存的 SDK 根路径与 UAP 版本号。
function get_sdk_info()
    return _sdk_root, _sdk_version
end

do
    _sdk_root, _sdk_version = _discover_sdk()
end
