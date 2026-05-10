-- Notepad WinUI3 demo with x:Bind
target("demo.notepad")
    add_rules("winui3.app", {namespace = "notepad", src_dir = path.join(os.scriptdir(), "src")})
    add_rules("demo.common")
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
