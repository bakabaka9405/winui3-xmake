-- xmake rule for Win2D (Microsoft.Graphics.Win2D) native DLL deployment
--
-- This rule copies Microsoft.Graphics.Canvas.dll from the NuGet package
-- to the target output directory after build. Only add this rule to
-- targets that actually depend on Win2D; targets without Win2D should
-- not include this rule to avoid unnecessary DLL copies.
--
-- Usage (in target xmake.lua):
--     add_rules("winui3.app", "demo.common", "win2d")

local _cached_win2d_path = nil

rule("win2d")
    after_build(function (target)
        -- Resolve Win2D NuGet package path from packages.config (cached at module level)
        if not _cached_win2d_path then
            local nuget_cfg = import("scripts.nuget_config", {rootdir = os.projectdir()})
            _cached_win2d_path = nuget_cfg.package_path("Microsoft.Graphics.Win2D")
        end

        if not _cached_win2d_path then
            raise("Microsoft.Graphics.Win2D NuGet package not found in packages.config")
        end

        -- Copy Microsoft.Graphics.Canvas.dll to target output directory
        local outdir = target:targetdir()
        local canvas_src = (_cached_win2d_path .. "/runtimes/win-x64/native/Microsoft.Graphics.Canvas.dll"):gsub("/", "\\")
        local canvas_dst = path.join(outdir, "Microsoft.Graphics.Canvas.dll")
        if os.isfile(canvas_src) then
            os.cp(canvas_src, canvas_dst)
        end
    end)
