-- MicroTeX 资源部署规则。
rule("microtex")
    after_build(function (target)
        local package = target:pkg("microtex")
        os.cp(
            path.join(package:installdir(), "res"),
            path.join(target:targetdir(), "res"),
            {copy_if_different = true}
        )
    end)
