-- Paint WinUI3 demo — Win2D canvas drawing application
target("demo.paint")
    add_rules("winui3.app", {namespace = "paint", src_dir = path.join(os.scriptdir(), "src")})
    add_rules("demo.common")
    add_rules("win2d")
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
