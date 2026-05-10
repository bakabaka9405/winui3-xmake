-- Camera WinUI3 demo
target("demo.camera")
    add_rules("winui3.app", {namespace = "camera", src_dir = path.join(os.scriptdir(), "src")})
    add_rules("demo.common")
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
