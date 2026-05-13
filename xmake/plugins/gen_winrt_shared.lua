-- xmake plugin: gen-winrt-shared
-- 生成共享 C++/WinRT 投影头（平台契约、WebView2、Windows App SDK、Win2D）。
-- 独立于具体目标，所有 WinUI 目标共用同一份输出目录。
--
-- Usage: xmake gen-winrt-shared

task("gen-winrt-shared")
    set_category("plugin")
    on_run(function ()
        local task = import("core.project.task")

        task.run("config")

        local utils = import("utils")

        local shared_gen = path.join(os.projectdir(), "build", ".gens", "shared", "generated")
        local script = path.join(os.projectdir(), "scripts/build_winui3_shared_projection.py")
        local args = {
            path.translate(script),
            "--project-dir",          path.translate(os.projectdir()),
            "--shared-projection-dir", path.translate(shared_gen),
        }

        utils.vcprint("${color.success}Running: Phase 0 (shared WinRT projection)")
        os.runv("python", args)
    end)

    set_menu {
        usage       = "xmake gen-winrt-shared",
        description = "Generate shared C++/WinRT projection headers for all WinUI targets.",
        options     = {}
    }
