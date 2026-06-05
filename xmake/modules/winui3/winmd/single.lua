-- 通用单 WinMD NuGet 包收集器，供 WebView2、Win2D 等包共享文件定位逻辑。
-- spec 表字段：
--   alias        (string):  NuGet 包 ID（如 "Microsoft.Web.WebView2"、"Microsoft.Graphics.Win2D"）
--   display_name (string):  显示名，用于错误消息（如 "WebView2"、"Win2D"）
--   winmd_path   (string):  包根目录下的 WinMD 相对路径

function collect(spec)
    local pkgs = import("winui3.packages")
    local root = pkgs.package_root(spec.alias)
    local winmd = path.join(root, spec.winmd_path)
    if not os.isfile(winmd) then
        raise(spec.display_name .. ": WinMD not found")
    end

    return {winmd}
end

