local depend = import("core.project.depend")
local option = import("core.base.option")


--- before_build 回调：在 XAML Pass 1/Pass 2 之后、编译之前执行。
---
--- 通过 add_orders("winui3.xaml", "winui3.pri")（在 xmake.lua 中声明）
--- 保证本回调在 winui3.xaml 的 before_build 之后运行；
--- add_deps 单独不足以保证 before_build 回调的执行顺序。
function before_build(target)
    local autogen_root  = target:autogendir({root = true})
    local generated_dir   = path.absolute(path.join(autogen_root, "generated"))
    local resfiles_path   = path.absolute(path.join(generated_dir, "layout.resfiles"))
    local config_xml_path = path.absolute(path.join(generated_dir, "priconfig.xml"))
    local pri_output      = path.absolute(path.join(generated_dir, "resources.pri"))

    local xbf_files = os.files(path.join(generated_dir, "*.xbf"))

    depend.on_changed(function()
        if not xbf_files or #xbf_files == 0 then
            raise("winui3.pri: no .xbf files found")
        end

        local tools = import("winui3.tools")()

        local resfiles_lines = {}
        for _, xbf in ipairs(xbf_files) do
            table.insert(resfiles_lines, path.absolute(xbf))
        end
        io.writefile(resfiles_path, table.concat(resfiles_lines, "\n"))

        local priconfig_xml = string.format([[
<?xml version="1.0" encoding="utf-8"?>
<resources targetOsVersion="10.0.0" majorVersion="1">
  <index root="%s" startIndexAt="%s">
    <default>
      <qualifier name="Language" value="en-US"/>
      <qualifier name="Contrast" value="standard"/>
      <qualifier name="Scale" value="200"/>
      <qualifier name="HomeRegion" value="001"/>
      <qualifier name="TargetSize" value="256"/>
      <qualifier name="LayoutDirection" value="LTR"/>
      <qualifier name="DXFeatureLevel" value="DX9"/>
      <qualifier name="Configuration" value=""/>
      <qualifier name="AlternateForm" value=""/>
      <qualifier name="Platform" value="UAP"/>
    </default>
    <indexer-config type="RESFILES" qualifierDelimiter="."/>
    <indexer-config type="EMBEDFILES"/>
  </index>
</resources>]], generated_dir, resfiles_path)
        io.writefile(config_xml_path, priconfig_xml)

        local project_dir = os.projectdir()
        os.vrunv(tools.makepri, {
            "new",
            "/cf", config_xml_path,
            "/pr", project_dir,
            "/o",
            "/of", pri_output,
        }, {envs = target:toolchain("msvc"):runenvs()})
    end, {
        files      = target:sourcebatches()["winui3.xaml"].sourcefiles,
        dependfile = path.join(target:dependir({root = true}), "pri.d"),
        changed    = option.get("rebuild"),
    })
end
