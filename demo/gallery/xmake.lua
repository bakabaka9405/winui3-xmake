-- WinUI3 Gallery demo — showcases major controls with visual parameter editors
target("demo.gallery")
    add_rules("winui3.app", "demo.common")
    set_values("winui3.namespace", "gallery")
    set_values("winui3.src_dir", path.join(os.scriptdir(), "src"))
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
