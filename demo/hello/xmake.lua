-- Hello World WinUI3 demo
target("demo.hello")
    add_rules("winui3.app", "demo.common")
    set_values("winui3.namespace", "hello")
    set_values("winui3.src_dir", path.join(os.scriptdir(), "src"))
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
