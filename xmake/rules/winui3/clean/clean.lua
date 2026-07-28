-- winui3.clean 规则的 after_clean 实现
--
-- 清理 WinUI3 构建流水线产生的中间文件和部署文件。
-- 生成目录（build/.gens/<target>/）包含：
--   - C++/WinRT 投影源码（.h, .cpp）
--   - XAML 编译产物（.xbf, .g.hpp, .g.cpp）
--   - WinMD 中间文件（.winmd）
--   - 资源索引文件（resources.pri）
--   - 其他构建中间文件
-- 指纹目录（build/.dep/<target>/）包含：
--   - 各规则的增量编译指纹文件
-- 部署目录（build/windows/x64/<mode>/<target>/）包含：
--   - 应用程序可执行文件（.exe）
--   - 运行时依赖文件（Bootstrap.dll, resources.pri）

local config = import("core.project.config")

function after_clean(target)
    local autogen_root = target:autogendir({root = true})
    os.rm(path.join(autogen_root, "*.rsp"))
    os.rm(path.join(autogen_root, "generated"))
    os.rm(path.join(autogen_root, "winmd_merged"))
    os.rm(path.join(autogen_root, "winmd_unmerged"))

    local depend_root = target:dependir({root = true})
    os.rm(path.join(depend_root, "*.d"))

    local deploy_dir = target:targetdir()
    if os.isdir(deploy_dir) then
        os.rm(deploy_dir)
    end
end
