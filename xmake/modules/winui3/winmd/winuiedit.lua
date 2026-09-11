function collect(opt)
    local single = import("winui3.winmd.single")
    return single.collect({
        alias        = "WinUIEdit",
        display_name = "WinUIEdit",
        winmd_path   = path.join("lib", "uap10.0", "WinUIEditor.winmd"),
    })
end
