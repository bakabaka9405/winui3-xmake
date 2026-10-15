function main(target)
    target:add("defines", "NOMINMAX", "WIN32_LEAN_AND_MEAN", "UNICODE", "_UNICODE")

    if target:toolchain("mingw") then
        target:add("includedirs", path.join(os.projectdir(), "common"))
        target:add("cxflags", "-fms-extensions", "-Wno-nonportable-include-path")
        if is_mode("debug") then
            target:add("defines", "_DEBUG")
        end

        -- MinGW 提供基础 Windows 头，SDK 补充 WinRT 声明。
        local sdk_root, sdk_version = import("winui3.sdk").get_sdk_info()
        for _, dir in ipairs({"um", "shared", "winrt"}) do
            target:add("cxflags", {"-idirafter", path.join(sdk_root, "Include", sdk_version, dir)}, {force = true, expand = false})
        end
    else
        target:add("cxflags", "/EHsc", "/bigobj", "/await:strict", "/utf-8")
    end
end
