-- Hello World WinUI3 demo
target("demo.hello")
    add_rules("winui3.app")
    set_values("winui3.namespace", "hello")
    add_rules("demo.common")
    add_includedirs("src")
    add_files("src/**.cpp", "src/**.idl", "src/**.xaml")
