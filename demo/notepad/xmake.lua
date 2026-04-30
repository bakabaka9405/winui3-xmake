-- Notepad WinUI3 demo with x:Bind
target("demo.notepad")
    add_rules("winui3.app", "demo.common")
    set_values("winui3.namespace", "notepad")
    set_values("winui3.src_dir", path.join(os.scriptdir(), "src"))
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
