-- 从 Windows App SDK 2.0.1+ 的三个子包中收集 WinMD 元数据文件：
--   Microsoft.WindowsAppSDK.Foundation          — metadata/*.winmd
--   Microsoft.WindowsAppSDK.WinUI               — metadata/*.winmd
--   Microsoft.WindowsAppSDK.InteractiveExperiences — metadata/<version>/*.winmd

local packages = import("winui3.packages")
local utils    = import("utils")

-- InteractiveExperiences 使用版本化 metadata 子目录，需选择最高版本目录。
local function _find_max_version_subdir(dir_path)
    if not os.isdir(dir_path) then
        return nil
    end

    local names   = {}
    local entries = {}
    for _, entry in ipairs(os.dirs(path.join(dir_path, "*"))) do
        local name = path.filename(entry)
        names[#names + 1] = name
        entries[name] = entry
    end
    local best_name = utils.max_version(names)
    return best_name and entries[best_name] or nil
end

function collect()
    local foundation_root = packages.package_root("Microsoft.WindowsAppSDK.Foundation")
    local foundation_meta = path.join(foundation_root, "metadata")
    if not os.isdir(foundation_meta) then
        raise(string.format("Foundation metadata 目录不存在：%s", foundation_meta))
    end
    local fnd_winmds = os.files(path.join(foundation_meta, "*.winmd"))
    if #fnd_winmds == 0 then
        raise(string.format("Foundation metadata 目录中无 .winmd 文件：%s", foundation_meta))
    end
    local winui_root = packages.package_root("Microsoft.WindowsAppSDK.WinUI")
    local winui_meta = path.join(winui_root, "metadata")
    if not os.isdir(winui_meta) then
        raise(string.format("WinUI metadata 目录不存在：%s", winui_meta))
    end
    local wui_winmds = os.files(path.join(winui_meta, "*.winmd"))
    if #wui_winmds == 0 then
        raise(string.format("WinUI metadata 目录中无 .winmd 文件：%s", winui_meta))
    end

    local ixp_root = packages.package_root("Microsoft.WindowsAppSDK.InteractiveExperiences")
    local ixp_meta_root = path.join(ixp_root, "metadata")
    if not os.isdir(ixp_meta_root) then
        raise(string.format("InteractiveExperiences metadata 目录不存在：%s", ixp_meta_root))
    end
    local ixp_version_dir = _find_max_version_subdir(ixp_meta_root)
    if not ixp_version_dir then
        raise(string.format("InteractiveExperiences metadata 下无版本子目录：%s", ixp_meta_root))
    end
    local ixp_winmds = os.files(path.join(ixp_version_dir, "*.winmd"))
    if #ixp_winmds == 0 then
        raise(string.format(
            "InteractiveExperiences metadata 版本目录中无 .winmd 文件：%s", ixp_version_dir))
    end

    local winmds = table.join(fnd_winmds, wui_winmds, ixp_winmds)
    return winmds
end

