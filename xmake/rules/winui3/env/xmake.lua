-- winui3.env 规则：目标身份、参数校验、包/编译/链接环境
rule("winui3.env")
    on_load(function (target)
        local env = import("env")
        env.on_load(target)
    end)

    before_prepare(function (target)
        local env = import("env")
        env.before_prepare(target)
    end)

    on_config(function (target)
        local env = import("env")
        env.on_config(target)
    end)
