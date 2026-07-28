-- C++/WinRT .g.cpp inclusion 模型：生成的 .g.cpp 不作为独立翻译单元，而由以下文件包含：
--     - *.xaml.cpp（如 MainWindow.xaml.cpp #include "MainWindow.g.cpp"）
--     - XamlMetaDataProvider.cpp（XMDP 规则生成，#include "XamlMetaDataProvider.g.cpp"）
-- 依赖上游规则与模块状态：
--   winui3.namespace                   — 目标根命名空间（由 set_values 提供，经 target:values 读取）
--   winui3.winmd.context                 — WinMD 上下文（SDK 与引用 WinMD）

local depend = import("core.project.depend")
local json_mod = import("core.base.json")
local winmd_context = import("winui3.winmd.context")
local option = import("core.base.option")


local function _msbuild_item(filepath, dependent_upon)
    local absolute = path.absolute(filepath)
    local item = {ItemSpec = absolute, FullPath = absolute}
    if dependent_upon then
        item.DependentUpon = path.absolute(dependent_upon)
    end
    return item
end


-- XAML Compiler 要求空集合编码为 JSON 数组；xmake 默认会将空表编码为对象。
local function _mark_empty_arrays(data, json_mod)
    for _, field in ipairs({"ReferenceAssemblies", "ReferenceAssemblyPaths", "XamlPages", "ClIncludeFiles"}) do
        if data[field] and #data[field] == 0 then
            json_mod.mark_as_array(data[field])
        end
    end
end

-- 每个 WinUI 3 目标必须且只能有一个 App.xaml，其余 XAML 均按页面输入传给编译器。
local function _classify_xaml(all_xaml)
    local app_xaml = nil
    local xaml_pages = {}

    for _, xf in ipairs(all_xaml) do
        local basename = path.filename(xf):lower()
        if basename == "app.xaml" then
            if app_xaml then
                raise("winui3.xaml: 发现多个 App.xaml 文件: " .. app_xaml .. " 与 " .. xf)
            end
            app_xaml = xf
        else
            table.insert(xaml_pages, xf)
        end
    end

    if not app_xaml then
        raise("cannot find App.xaml")
    end

    return app_xaml, xaml_pages
end

local function _discover_header_files(all_xaml)
    local header_files = {}
    for _, xf in ipairs(all_xaml) do
        local hf = xf:gsub("%.xaml$", ".xaml.h")
        if os.isfile(hf) then
            table.insert(header_files, {hf = hf, xf = xf})
        end
    end
    return header_files
end

function before_prepare_files(target, sourcebatch, opt)
    local all_xaml = sourcebatch.sourcefiles
    if not all_xaml or #all_xaml == 0 then return end
    local shared = winmd_context.ensure(target)
    local namespace = target:values("winui3.namespace")
    local autogen_root  = target:autogendir({root = true})
    local generated_dir = path.join(autogen_root, "generated")

    -- 收集所有指纹跟踪输入（.xaml 源文件 + 关联 .xaml.h 头文件）
    local header_files = _discover_header_files(all_xaml)
    local input_files = table.join(all_xaml)
    for _, hfp in ipairs(header_files) do
        table.insert(input_files, hfp.hf)
    end

    -- 仅在 XAML 源文件或关联头文件变更时执行 Pass1（生成 .xbf）和 Pass2（生成 .g.hpp / .g.cpp）
    depend.on_changed(function()
        local tools = import("winui3.tools")()
        local xaml_compiler = tools.xaml_compiler
        local genxbf_dir    = tools.genxbf_dir

        local ref_winmds = shared.ref_winmds
        local sdk_version = shared.sdk_version
        if not sdk_version then raise("winui3.xaml: cannot determine Windows SDK version") end

        local app_xaml, xaml_pages = _classify_xaml(all_xaml)

        local pass1_data = {
            SavedStateFile         = path.absolute(path.join(generated_dir, "XamlCompilerState.xml")),
            IsPass1                = true,
            Language               = "CppWinRT",
            ProjectPath            = path.absolute(app_xaml),
            LanguageSourceExtension = ".cpp",
            OutputPath             = path.absolute(generated_dir),
            RootNamespace          = namespace,
            FeatureControlFlags    = "EnableXBindDiagnostics;EnableDefaultValidationContextGeneration;EnableWin32Codegen",
            ReferenceAssemblies    = {},
            ReferenceAssemblyPaths = {},
            TargetPlatformMinVersion = sdk_version,
            XamlPages              = {},
            XamlApplications       = {_msbuild_item(app_xaml)},
            ClIncludeFiles         = {},
        }
        for _, wm in ipairs(ref_winmds) do
            table.insert(pass1_data.ReferenceAssemblies, _msbuild_item(wm))
        end
        for _, xp in ipairs(xaml_pages) do
            table.insert(pass1_data.XamlPages, _msbuild_item(xp))
        end
        for _, hfp in ipairs(header_files) do
            table.insert(pass1_data.ClIncludeFiles, _msbuild_item(hfp.hf, hfp.xf))
        end
        _mark_empty_arrays(pass1_data, json_mod)
        io.writefile(path.join(generated_dir, "pass1.json"), json_mod.encode(pass1_data))
        os.vrunv(xaml_compiler, {path.join(generated_dir, "pass1.json"), path.join(generated_dir, "pass1_out.json")}, {envs = target:toolchain("msvc"):runenvs()})

        local merged_winmd = path.join(autogen_root, "winmd_merged", namespace .. ".winmd")
        if not os.isfile(merged_winmd) then
            raise(string.format(
                "winui3.xaml: Phase 7 (XAML Pass 2) requires merged WinMD not found: %s",
                merged_winmd))
        end

        -- Pass2: 在 Pass1 JSON 基础上修补差异字段
        pass1_data.IsPass1 = false
        pass1_data.LocalAssembly = {_msbuild_item(merged_winmd)}
        pass1_data.GenXbfPath = genxbf_dir
        io.writefile(path.join(generated_dir, "pass2.json"), json_mod.encode(pass1_data))
        os.vrunv(xaml_compiler, {path.join(generated_dir, "pass2.json"), path.join(generated_dir, "pass2_out.json")}, {envs = target:toolchain("msvc"):runenvs()})
    end, {
        files      = input_files,
        dependfile = path.join(target:dependir({root = true}), "xaml.d"),
        changed    = option.get("rebuild"),
    })
end
