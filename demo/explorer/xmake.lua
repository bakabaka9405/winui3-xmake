target("demo.explorer")
    add_rules("winui3.app", {namespace = "explorer", src_dir = path.join(os.scriptdir(), "src")})
    add_rules("demo.common")
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
