-- WinUI3 Gallery demo — showcases major controls with visual parameter editors
target("demo.gallery")
    add_rules("winui3.app")
    set_values("winui3.namespace", "gallery")
    add_rules("demo.common")
    add_includedirs("src")
    add_files("src/**.cpp", "src/**.idl", "src/**.xaml")
