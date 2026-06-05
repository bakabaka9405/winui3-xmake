-- 解析 WinUI3 构建管线所需的外部工具路径：
--   - midl.exe       — MIDL 编译器
--   - mdmerge.exe    — WinMD 合并工具
--   - cppwinrt.exe   — C++/WinRT 投影编译器
--   - makepri.exe    — 资源索引工具
--   - XamlCompiler.exe — XAML 编译器（可被用户覆盖）
-- 所有工具默认均从 NuGet 包解析，不依赖 MSVC 工具链安装。

local packages = import("winui3.packages")
local config   = import("core.project.config")
local utils    = import("utils")

-- 解析 BuildTools 中版本号最大的 x64 目录
local _bt_bin = path.join(packages.package_root("Microsoft.Windows.SDK.BuildTools"), "bin")
local _bt_ver, _bt_dir = nil, nil; do
    local names    = {}
    local x64_path = {}
    for _, e in ipairs(os.dirs(path.join(_bt_bin, "*"))) do
        local d = path.join(e, "x64")
        if os.isdir(d) then
            local n = path.filename(e)
            names[#names + 1] = n
            x64_path[n] = d
        end
    end
    local best_ver = utils.max_version(names)
    if best_ver then
        _bt_ver, _bt_dir = best_ver, x64_path[best_ver]
    end
end
if not _bt_dir then
    raise(string.format("BuildTools 中无 x64 版本目录: %s", _bt_bin))
end

local _wuit_dir = path.join(packages.package_root("Microsoft.WindowsAppSDK.WinUI"), "tools")

-- BuildTools 中的可执行文件
local _midl; do
    -- midlrt.exe 具备 WinRT 感知能力，优先于兼容封装 midl.exe
    for _, e in ipairs({"midlrt.exe", "midl.exe"}) do
        local p = path.join(_bt_dir, e)
        if os.isfile(p) then
            _midl = p
            break
        end
    end
    if not _midl then
        raise("winui3.tools: midlrt.exe / midl.exe 不在 " .. _bt_dir)
    end
end

local _mdmerge = path.join(_bt_dir, "mdmerge.exe")
if not os.isfile(_mdmerge) then
    raise("winui3.tools: mdmerge.exe 不在 " .. _bt_dir)
end

local _makepri = path.join(_bt_dir, "makepri.exe")
if not os.isfile(_makepri) then
    raise("winui3.tools: makepri.exe 不在 " .. _bt_dir)
end

-- cppwinrt.exe
local _cppwinrt = path.join(packages.package_root("Microsoft.Windows.CppWinRT"), "bin", "cppwinrt.exe")
if not os.isfile(_cppwinrt) then
    raise("winui3.tools: cppwinrt.exe 不在 NuGet 包中")
end

-- XamlCompiler.exe（支持配置项或环境变量覆盖）
local _xaml_compiler; do
    local candidates = {
        config.get("winui3_xaml_compiler_path"),
        os.getenv("WINUI3_XAML_COMPILER_PATH"),
        path.join(_wuit_dir, "net472"),
    }

    for _, c in ipairs(candidates) do
        local p = path.join(c, "XamlCompiler.exe")
        if os.isfile(p) then
            _xaml_compiler = p
            break
        end
    end

    if not _xaml_compiler then
        raise("cannot locate XamlCompiler.exe")
    end
end

local _tools = {
    midl          = _midl,
    mdmerge       = _mdmerge,
    makepri       = _makepri,
    cppwinrt      = _cppwinrt,
    genxbf_dir    = _wuit_dir,
    xaml_compiler = _xaml_compiler,
}

function main()
    return _tools
end
