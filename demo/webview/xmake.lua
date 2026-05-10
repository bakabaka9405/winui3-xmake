-- WebView2 Hello World WinUI3 demo
target("demo.webview")
    add_rules("winui3.app", {namespace = "webview", src_dir = path.join(os.scriptdir(), "src")})
    add_rules("demo.common")
    add_rules("webview", {dist_dir = path.join(os.scriptdir(), "src", "web")})
    add_files("src/**.cpp")
