local function json_escape(value)
    local text = value or ""
    text = text:gsub("\\", "\\\\")
    text = text:gsub("\"", "\\\"")
    text = text:gsub("\r", "\\r")
    text = text:gsub("\n", "\\n")
    return text
end

local function json_string(value)
    return "\"" .. json_escape(value) .. "\""
end

local function json_array(items)
    return "[" .. table.concat(items, ",") .. "]"
end

local function make_msbuild_item(path_value, depends_value)
    local fields = {
        "\"ItemSpec\":" .. json_string(path_value),
        "\"FullPath\":" .. json_string(path_value)
    }
    if depends_value then
        table.insert(fields, "\"DependentUpon\":" .. json_string(depends_value))
    end
    return "{" .. table.concat(fields, ",") .. "}"
end

local function ps_single_quote(value)
    return (value or ""):gsub("'", "''")
end

local function batch_write_file(batchcmds, file_path, content)
    local script = "$text=@'\n" ..
        content ..
        "\n'@; Set-Content -LiteralPath '" ..
        ps_single_quote(path.translate(file_path)) .. "' -Value $text -Encoding UTF8"
    batchcmds:vrunv("powershell", {
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-Command",
        script
    })
end

local function xaml_json_input(ctx, is_pass1)
    local ref_assemblies = {}
    for _, ref_path in ipairs(ctx.winmd_refs) do
        table.insert(ref_assemblies, make_msbuild_item(ref_path))
    end

    local xaml_pages = {}
    for _, page_path in ipairs(ctx.xaml_pages) do
        table.insert(xaml_pages, make_msbuild_item(page_path))
    end

    local xaml_apps = {}
    for _, app_path in ipairs(ctx.xaml_apps) do
        table.insert(xaml_apps, make_msbuild_item(app_path))
    end

    local cl_include_files = {}
    for _, xaml_path in ipairs(ctx.xaml_apps) do
        table.insert(cl_include_files, make_msbuild_item(xaml_path .. ".h", xaml_path))
    end
    for _, xaml_path in ipairs(ctx.xaml_pages) do
        table.insert(cl_include_files, make_msbuild_item(xaml_path .. ".h", xaml_path))
    end

    local fields = {
        "\"SavedStateFile\":" .. json_string(ctx.xaml_saved_state),
        "\"IsPass1\":" .. (is_pass1 and "true" or "false"),
        "\"Language\":" .. json_string("CppWinRT"),
        "\"ProjectPath\":" .. json_string(ctx.project_script),
        "\"LanguageSourceExtension\":" .. json_string(".cpp"),
        "\"OutputPath\":" .. json_string(ctx.xaml_generated_dir),
        "\"RootNamespace\":" .. json_string(ctx.root_namespace),
        "\"PrecompiledHeaderFile\":" .. json_string("pch.h"),
        "\"FeatureControlFlags\":" ..
        json_string("EnableXBindDiagnostics;EnableDefaultValidationContextGeneration;EnableWin32Codegen"),
        "\"ReferenceAssemblies\":" .. json_array(ref_assemblies),
        "\"ReferenceAssemblyPaths\":[]",
        "\"TargetPlatformMinVersion\":" .. json_string(ctx.sdk_version),
        "\"XamlPages\":" .. json_array(xaml_pages),
        "\"XamlApplications\":" .. json_array(xaml_apps),
        "\"ClIncludeFiles\":" .. json_array(cl_include_files)
    }

    if not is_pass1 then
        table.insert(fields, "\"LocalAssembly\":" .. json_array({ make_msbuild_item(ctx.merged_winmd) }))
        table.insert(fields, "\"GenXbfPath\":" .. json_string(ctx.appsdk_tools_dir))
    end

    return "{" .. table.concat(fields, ",") .. "}"
end

function write_xaml_input_json(batchcmds, ctx)
    batch_write_file(batchcmds, ctx.xaml_pass1_in_json, xaml_json_input(ctx, true))
    batch_write_file(batchcmds, ctx.xaml_pass2_in_json, xaml_json_input(ctx, false))
end

function write_makepri_inputs(batchcmds, ctx)
    local resfiles = table.concat(ctx.xaml_xbf_files, "\n")
    batch_write_file(batchcmds, ctx.makepri_resfiles, resfiles)

    local xml_lines = {
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>",
        "<resources targetOsVersion=\"10.0.0\" majorVersion=\"1\">",
        "<index root=\"" ..
        json_escape(ctx.xaml_generated_dir) .. "\" startIndexAt=\"" .. json_escape(ctx.makepri_resfiles) .. "\">",
        "<default>",
        "<qualifier name=\"Language\" value=\"en-US\" />",
        "<qualifier name=\"Contrast\" value=\"standard\" />",
        "<qualifier name=\"Scale\" value=\"200\" />",
        "<qualifier name=\"HomeRegion\" value=\"001\" />",
        "<qualifier name=\"TargetSize\" value=\"256\" />",
        "<qualifier name=\"LayoutDirection\" value=\"LTR\" />",
        "<qualifier name=\"DXFeatureLevel\" value=\"DX9\" />",
        "<qualifier name=\"Configuration\" value=\"\" />",
        "<qualifier name=\"AlternateForm\" value=\"\" />",
        "<qualifier name=\"Platform\" value=\"UAP\" />",
        "</default>",
        "<indexer-config type=\"RESFILES\" qualifierDelimiter=\".\" />",
        "<indexer-config type=\"EMBEDFILES\" />",
        "</index>",
        "</resources>"
    }
    batch_write_file(batchcmds, ctx.makepri_config, table.concat(xml_lines, "\n"))
end
