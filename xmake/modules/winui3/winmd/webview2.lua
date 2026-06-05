function collect(opt)
    local single = import("winui3.winmd.single")
    return single.collect({
        alias        = "Microsoft.Web.WebView2",
        display_name = "WebView2",
        winmd_path   = path.join("lib", "Microsoft.Web.WebView2.Core.winmd"),
    })
end

