local option = import("core.base.option")

function vcprint(format, ...)
    if option.get("verbose") then
        cprint(format, ...)
    end
end
