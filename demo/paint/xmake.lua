-- Paint WinUI3 demo — Win2D canvas drawing application
target("demo.paint")
    add_rules("winui3.app", "winui3.modules")
    set_values("winui3.namespace", "paint")
    add_rules("demo.common")
    add_includedirs("src")
    add_rules("win2d")
    add_files("src/**.cpp", "src/**.idl", "src/**.xaml")
