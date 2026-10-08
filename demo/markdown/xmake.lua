-- Markdown 预览器 WinUI 3 示例
target("demo.markdown")
    add_rules("winui3.app", "winui3.modules")
    set_values("winui3.namespace", "markdown")
    add_rules("demo.common", "winuiedit", "win2d", "microtex")
    add_packages("md4c", "microtex")
    add_includedirs("src")
    add_files("src/**.cpp", "src/**.cppm", "src/**.idl", "src/**.xaml")
