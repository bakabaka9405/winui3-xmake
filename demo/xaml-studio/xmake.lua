-- XAML Studio WinUI3 demo
target("demo.xaml-studio")
    add_rules("winui3.app", {namespace = "xamlstudio", src_dir = path.join(os.scriptdir(), "src")})
    add_rules("demo.common")
    add_rules("webview", {dist_dir = path.join(os.scriptdir(), "src", "web")})
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
