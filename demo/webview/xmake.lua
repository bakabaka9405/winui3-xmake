-- WebView2 Hello World WinUI3 demo
target("demo.webview")
    add_rules("winui3.app", "demo.common", "webview")
    set_values("winui3.namespace", "webview")
    set_values("winui3.src_dir", path.join(os.scriptdir(), "src"))
    add_files("src/**.cpp")
