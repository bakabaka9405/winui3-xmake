-- XAML Studio WinUI3 demo
target("demo.xaml-studio")
    add_rules("winui3.app")
    set_values("winui3.namespace", "xamlstudio")
    add_rules("demo.common")
    add_includedirs("src")
    add_rules("webview", {dist_dir = path.join(os.scriptdir(), "src", "web")})
    add_files("src/**.cpp", "src/**.idl", "src/**.xaml")
