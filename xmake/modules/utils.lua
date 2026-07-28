local option = import("core.base.option")

function vcprint(format, ...)
    if option.get("verbose") then
        cprint(format, ...)
    end
end

-- 外部工具响应文件写入
function write_response_file(target, filename, args)
    local rsp = path.join(target:autogendir({root = true}), filename)
    local lines = {}
    for _, arg in ipairs(args) do
        local value = tostring(arg)
        if value:find("[%s]") then
            table.insert(lines, '"' .. value .. '"')
        else
            table.insert(lines, value)
        end
    end
    io.writefile(rsp, table.concat(lines, "\r\n"))
    return rsp
end

--- 仅在内容变化时写入文件，保持未变文件的 mtime，避免增量构建重复触发。
---
---@param filepath string 文件路径
---@param content string 文件内容
---@return boolean 是否发生了写入（内容有变化时返回 true，无变化时返回 false）
function write_file_if_changed(filepath, content)
    if os.isfile(filepath) then
        local existing = io.readfile(filepath)
        if existing == content then
            return false
        end
    end
    io.writefile(filepath, content)
    return true
end

--- 从版本字符串列表中选取最大版本号（按 4 段数值逐段比较）。
---
--- 仅处理符合 X.Y.Z.W 格式的四段版本号，跳过不符合格式的字符串。
--- 比较方式：按 `.` 分割后对各段做数值比较，确保 10.0.10.0 > 10.0.9.0。
---
---@param versions string[] 候选版本号列表
---@return string|nil        最大版本号，若列表中无合法版本号则返回 nil
function max_version(versions)
    --- 解析四段版本号为数值表，不符合格式则返回 nil。
    local function _parse(ver)
        local v1, v2, v3, v4 = ver:match("^(%d+)%.(%d+)%.(%d+)%.(%d+)$")
        if not v1 then
            return nil
        end
        return {tonumber(v1), tonumber(v2), tonumber(v3), tonumber(v4)}
    end

    local best_str = nil
    local best_parts = nil

    for _, ver in ipairs(versions) do
        local parts = _parse(ver)
        if parts then
            if not best_parts then
                best_str, best_parts = ver, parts
            else
                for i = 1, 4 do
                    if parts[i] > best_parts[i] then
                        best_str, best_parts = ver, parts
                        break
                    elseif parts[i] < best_parts[i] then
                        break
                    end
                end
            end
        end
    end

    return best_str
end
