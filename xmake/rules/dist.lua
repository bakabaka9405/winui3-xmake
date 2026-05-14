rule("mode.dist")
	after_load(function (target)
		if not is_mode("dist") then
            return
        end
		target:set("symbols", "hidden")
		target:set("optimize", "fastest")
		target:set("strip", "all")
		target:set("runtimes", "MT")
	end)