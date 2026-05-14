-- xmake rule for demo project shared configuration
--
-- This rule handles project-specific shared patterns that are NOT universal
-- to all WinUI3 applications — specifically the common/ directory that
-- contains shared entry point, precompiled header, and manifest files.
--
-- Usage in demo xmake.lua:
--     target("my_demo")
--         add_rules("winui3.app", "demo.common")
--         ...

rule("demo.common")

    on_load(function (target)
        -- target:set("default", false)
        local common_dir = path.join(os.projectdir(), "common")
        target:add("includedirs", common_dir)
        target:add("files", path.join(common_dir, "**.cpp"))
        target:add("files", path.join(common_dir, "**.manifest"))
    end)
