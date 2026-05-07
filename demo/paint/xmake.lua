-- Paint WinUI3 demo — Win2D canvas drawing application
target("demo.paint")
    add_rules("winui3.app", "demo.common", "win2d")
    set_values("winui3.namespace", "paint")
    set_values("winui3.src_dir", path.join(os.scriptdir(), "src"))
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
