-- Camera WinUI3 demo
target("demo.camera")
    add_rules("winui3.app")
    set_values("winui3.namespace", "camera")
    add_rules("demo.common")
    add_includedirs("src")
    add_files("src/**.cpp", "src/**.idl", "src/**.xaml")
