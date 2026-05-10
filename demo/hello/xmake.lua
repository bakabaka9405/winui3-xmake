-- Hello World WinUI3 demo
target("demo.hello")
    add_rules("winui3.app", {namespace = "hello", src_dir = path.join(os.scriptdir(), "src")})
    add_rules("demo.common")
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
