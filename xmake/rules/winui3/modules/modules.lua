-- winui3.modules 规则实现

function on_load(target)
    target:set("policy", "build.c++.modules", true)
    target:add("deps", "winui3.shared_projection.modules")
end

function on_config(target)
    local namespace = target:values("winui3.namespace")
    target:add("defines", "WINRT_ENABLE_LEGACY_COM", "WINUI3_IMPORT_MODULE=" .. namespace .. ".winrt")
end
