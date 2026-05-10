-- WinUI3 Gallery demo — showcases major controls with visual parameter editors
target("demo.gallery")
    add_rules("winui3.app", {namespace = "gallery", src_dir = path.join(os.scriptdir(), "src")})
    add_rules("demo.common")
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
