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

rule("demo.msvc_runtime")
    after_build(function (target)
        if not is_mode("release") then
            return
        end

        -- Normalize Visual Studio paths before combining them with xmake paths.
        local function normalize_dir(dir)
            if dir and #dir > 0 then
                return dir:gsub("\\", "/"):gsub("/+$", "")
            end
        end

        local function append_redist_root(roots, dir)
            dir = normalize_dir(dir)
            if dir then
                table.insert(roots, dir)
            end
        end

        local function redist_arch()
            local arch = target:arch() or "x64"
            if arch == "x86" or arch == "i386" then
                return "x86"
            elseif arch == "arm64" then
                return "arm64"
            else
                return "x64"
            end
        end

        -- Locate the redistributable CRT directories from the active VS environment
        -- first, then fall back to standard Visual Studio installation layouts.
        local function collect_crt_dirs()
            local arch = redist_arch()
            local roots = {}

            append_redist_root(roots, os.getenv("VCToolsRedistDir"))

            local vcinstall = normalize_dir(os.getenv("VCINSTALLDIR"))
            if vcinstall then
                append_redist_root(roots, path.join(vcinstall, "Redist", "MSVC"))
            end

            local vsinstall = normalize_dir(os.getenv("VSINSTALLDIR"))
            if vsinstall then
                append_redist_root(roots, path.join(vsinstall, "VC", "Redist", "MSVC"))
            end

            local program_files = normalize_dir(os.getenv("ProgramFiles")) or "C:/Program Files"
            local program_files_x86 = normalize_dir(os.getenv("ProgramFiles(x86)")) or "C:/Program Files (x86)"

            for _, base in ipairs({ program_files, program_files_x86 }) do
                for _, edition in ipairs(os.dirs(path.join(base, "Microsoft Visual Studio", "*", "*"))) do
                    append_redist_root(roots, path.join(edition, "VC", "Redist", "MSVC"))
                end
            end

            local crt_dirs = {}
            for _, root in ipairs(roots) do
                for _, dir in ipairs(os.dirs(path.join(root, arch, "Microsoft.VC*.CRT"))) do
                    table.insert(crt_dirs, dir)
                end
                for _, dir in ipairs(os.dirs(path.join(root, "*", arch, "Microsoft.VC*.CRT"))) do
                    table.insert(crt_dirs, dir)
                end
            end

            table.sort(crt_dirs)
            return crt_dirs
        end

        local outdir = target:targetdir()
        local crt_dirs = collect_crt_dirs()

        for i = #crt_dirs, 1, -1 do
            local dlls = os.files(path.join(crt_dirs[i], "*.dll"))
            if #dlls > 0 then
                for _, dll in ipairs(dlls) do
                    os.cp(dll, path.join(outdir, path.filename(dll)))
                end
                cprint("${bright}demo:${clear} copied MSVC runtime DLLs from %s", crt_dirs[i])
                return
            end
        end

        cprint("${yellow}warning:${clear} MSVC runtime DLLs were not found; install the latest Visual C++ Redistributable on target machines or run from a Visual Studio developer environment.")
    end)

rule("demo.common")
    add_deps("demo.msvc_runtime")

    on_load(function (target)
        target:set("default", false)

        local common_dir = path.join(os.projectdir(), "common")
        target:add("includedirs", common_dir)
        target:add("files", path.join(common_dir, "**.cpp"))
        target:add("files", path.join(common_dir, "**.manifest"))
    end)
