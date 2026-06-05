-- 从 Windows SDK 的 Platform.xml 解析 API 契约，并按 References 布局收集对应 WinMD。
-- SDK 路径发现委托给 winui3.sdk。

local platform_winmds = nil
local metadata_dir = nil

local sdk   = import("winui3.sdk")
local utils = import("utils")

-- Platform.xml 中的 ApiContract 条目足够规整，可用 Lua 模式匹配避免引入 XML 解析依赖。
local function _parse_contracts(platform_xml_path)
    local content=io.readfile(platform_xml_path)

    if not content or #content == 0 then
        raise(string.format("Platform.xml 为空：%s", platform_xml_path))
    end

    local contracts = {}
    for name, version in content:gmatch('ApiContract[^>]-name="([^"]+)"[^>]-version="([^"]+)"') do
        table.insert(contracts, {name = name, version = version})
    end

    -- 兼容 name/version 属性顺序相反的 Platform.xml 条目。
    if #contracts == 0 then
        for name, version in content:gmatch('ApiContract[^>]-version="([^"]+)"[^>]-name="([^"]+)"') do
            table.insert(contracts, {name = version, version = name})
        end
    end

    if #contracts == 0 then
        raise(string.format("Platform.xml 中未找到 ApiContract 条目：%s", platform_xml_path))
    end

    return contracts
end

function collect()
    return platform_winmds
end

-- MIDL /metadata_dir 需要 FoundationContract 的具体版本目录。
function find_metadata_dir()
    return metadata_dir
end

-- 发现 Windows SDK、收集平台 WinMD、定位 FoundationContract 元数据目录。
do
    local sdk_root, sdk_version = sdk.get_sdk_info()

    -- 定位 Platform.xml。
    local platform_xml = path.join(sdk_root, "Platforms", "UAP", sdk_version, "Platform.xml")
    if not os.isfile(platform_xml) then
        local uap_ver = sdk.discover_uap_version(sdk_root)
        if uap_ver then
            sdk_version = uap_ver
            platform_xml = path.join(sdk_root, "Platforms", "UAP", sdk_version, "Platform.xml")
        end
    end

    if not os.isfile(platform_xml) then
        raise(string.format("Windows SDK Platform.xml 不存在：%s", platform_xml))
    end

    -- 解析契约并收集 WinMD。
    local contracts = _parse_contracts(platform_xml)
    local winmds = {}
    for _, contract in ipairs(contracts) do
        local winmd = path.join(sdk_root, "References", sdk_version,
                contract.name, contract.version, contract.name .. ".winmd")
        if os.isfile(winmd) then
            table.insert(winmds, winmd)
        else
            raise(string.format(
                "平台 WinMD 文件不存在：%s\n  契约: %s v%s",
                winmd, contract.name, contract.version))
        end
    end
    table.sort(winmds)

    -- 查找 MIDL /metadata_dir 所需的 FoundationContract 版本目录。
    local foundation_root = path.join(sdk_root, "References", sdk_version,
        "Windows.Foundation.FoundationContract")
    if not os.isdir(foundation_root) then
        raise(string.format("FoundationContract 目录不存在：%s", foundation_root))
    end

    local names   = {}
    local entries = {}
    for _, entry in ipairs(os.dirs(path.join(foundation_root, "*"))) do
        local name = path.filename(entry)
        names[#names + 1] = name
        entries[name] = entry
    end
    local best_name = utils.max_version(names)
    if not best_name then
        raise(string.format("FoundationContract 下无版本目录：%s", foundation_root))
    end

    platform_winmds = winmds
    metadata_dir = entries[best_name]
end
