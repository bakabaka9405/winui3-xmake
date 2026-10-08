-- 共享 C++/WinRT 投影生成实现
--
-- 工具与 WinMD 解析均通过模块（winui3.tools / winui3.winmd.* / winui3.packages）。
-- WinMD 上下文由 winui3.winmd.context 持有，供下游规则复用。
--
-- 投影按 WinMD 节点独立输出到 build/.gens/shared/<node>/generated。
--
-- 每个节点的 WinMD 作为 -in，其传递依赖 WinMD 作为 -ref。
--
-- generate 在两种场景下调用：
--   1. winui3.shared_projection 规则（普通目标）：生成目标全部激活节点的投影头
--   2. winui3.shared_projection.modules.<node> 目标：生成单个节点的 .ixx 模块


local depend = import("core.project.depend")
local winmd_context = import("winui3.winmd.context")
local option = import("core.base.option")
local config = import("core.project.config")


-- 按拓扑序计算每个节点的传递依赖 WinMD。
local function _transitive_refs(graph_ctx)
    local dep_refs = {}
    for _, id in ipairs(graph_ctx.order) do
        local spec = graph_ctx.by_id[id]
        local result = {}
        for _, dep_id in ipairs(spec.deps) do
            local dep_spec = graph_ctx.by_id[dep_id]
            if dep_spec then
                table.join2(result, dep_spec.winmds)
                table.join2(result, dep_refs[dep_id] or {})
            end
        end
        dep_refs[id] = result
    end
    return dep_refs
end


local function _generate_node(target, graph_ctx, node_id, opt)
    local spec = graph_ctx.by_id[node_id]
    if not spec or #spec.winmds == 0 then
        return
    end

    local out_dir = winmd_context.node_dir(node_id)
    local winrt_dir = winmd_context.node_winrt_dir(node_id)

    local changed = option.get("rebuild") or false
    if not changed then
        -- 模块模式以 .ixx 作为存在性探针，普通模式以 base.h 作为探针。
        local probe = opt.modules and path.join(winrt_dir, "*.ixx")
            or path.join(winrt_dir, "base.h")
        if #os.files(probe) == 0 then
            changed = true
        end
    end

    local dep_refs = _transitive_refs(graph_ctx)
    local tools = import("winui3.tools")()
    local depend_files = table.copy(spec.winmds)
    table.join2(depend_files, dep_refs[node_id])

    depend.on_changed(function()
        os.mkdir(out_dir)

        local args = {}
        for _, wm in ipairs(spec.winmds) do
            table.insert(args, "-in")
            table.insert(args, wm)
        end
        for _, wm in ipairs(dep_refs[node_id]) do
            table.insert(args, "-ref")
            table.insert(args, wm)
        end
        if opt.modules then
            table.insert(args, "-modules")
        end
        table.insert(args, "-out")
        table.insert(args, out_dir)
        os.vrunv(tools.cppwinrt, args)

        -- Clang 的模块导入方需要可见的原生声明。
        if node_id == "platform" then
            local base_file = path.join(winrt_dir, "base.h")
            local base_content, base_count = io.readfile(base_file):gsub('extern "C"(%s*{)',
                '#ifdef WINRT_IMPL_BUILD_MODULE\nexport\n#endif\nextern "C"%1', 1)
            assert(base_count == 1, "C++/WinRT base.h native declaration block not found")
            io.writefile(base_file, base_content)

            if opt.modules then
                local foundation_file = path.join(winrt_dir, "winrt.Windows.Foundation.ixx")
                local foundation_content, foundation_count = io.readfile(foundation_file):gsub("module;",
                    "module;\n#include <guiddef.h>", 1)
                assert(foundation_count == 1, "C++/WinRT Foundation global module fragment not found")
                io.writefile(foundation_file, foundation_content)
            end
        end

        -- cppwinrt -modules 每次都会生成 winrt_base / winrt_numerics；各节点分别编译时会重复定义同名模块。
        -- 所有其他节点都依赖 platform，因此仅保留 platform 的副本，由其向下游提供这两个基础 BMI。
        if opt.modules and node_id ~= "platform" then
            os.tryrm(path.join(winrt_dir, "winrt_base.ixx"))
            os.tryrm(path.join(winrt_dir, "winrt_numerics.ixx"))
        end

        return {}
    end, {
        files      = depend_files,
        dependfile = path.join(config.builddir(), ".deps", "shared",
            node_id .. (opt.modules and ".modules" or ".headers") .. ".d"),
        changed    = changed,
    })
end


--- opt.node：仅生成指定节点（供独立模块目标使用）。
--- opt.modules：生成 C++ modules（.ixx）而非仅头文件。
function generate(target, opt)
    opt = opt or {}
    local shared = winmd_context.ensure(target)
    local graph_ctx = shared.graph

    local nodes = opt.node and {opt.node} or shared.nodes
    for _, node_id in ipairs(nodes) do
        _generate_node(target, graph_ctx, node_id, opt)
    end
end


function before_prepare(target)
    -- C++ modules 目标的投影由 winui3.shared_projection.modules.<node> 生成。
    if target:rule("winui3.modules") == nil then
        generate(target)
    end
end
