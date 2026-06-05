function collect(opt)
    local single = import("winui3.winmd.single")
    return single.collect({
        alias        = "Microsoft.Graphics.Win2D",
        display_name = "Win2D",
        winmd_path   = path.join("lib", "uap10.0", "Microsoft.Graphics.Canvas.winmd"),
    })
end

