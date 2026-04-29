-- Hello World WinUI3 demo
target("hello_demo")
    add_rules("winui3.app", "demo.common")
    set_values("winui3.namespace", "hello")
    set_values("winui3.src_dir", path.join(os.scriptdir(), "src"))
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
