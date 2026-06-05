-- Notepad WinUI3 demo with x:Bind
target("demo.notepad")
    add_rules("winui3.app")
    set_values("winui3.namespace", "notepad")
    add_rules("demo.common")
    add_includedirs("src")
    add_files("src/**.cpp", "src/**.idl", "src/**.xaml")
