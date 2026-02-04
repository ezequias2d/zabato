
local ZShader = {}


local active_env = nil
local proxy_env = {}

setmetatable(proxy_env, {
    __index = function(t, k)
        if active_env then
            local val = active_env[k]
            if val ~= nil then return val end
        end
        return _G[k]
    end,
    __newindex = function(t, k, v)
        if active_env then
             active_env[k] = v
        else
             _G[k] = v
        end
    end
})

function ZShader.new_cpu_context()
    return require("zshader.contexts.cpu")()
end

function ZShader.load_file(path)
    local chunk, err = loadfile(path, "t", proxy_env)
    if not chunk then error("Failed to load shader file: " .. tostring(err)) end
    return chunk()
end

function ZShader.load_string(code, chunkname)
    local chunk, err = load(code, chunkname, "t", proxy_env)
    if not chunk then error("Failed to load shader string: " .. tostring(err)) end
    return chunk()
end

function ZShader.compile(shader_def)
    local create_recorder = require("zshader.contexts.recorder")
    
    local reflection = {
        uniforms = {},
        attributes = {},
        varyings = {}
    }

    local function merge_reflection(data)
        if not data then return end
        for k,_ in pairs(data.uniform) do reflection.uniforms[k] = true end
        for k,_ in pairs(data.attribute) do reflection.attributes[k] = true end
        for k,_ in pairs(data.varying) do reflection.varyings[k] = true end
    end

    local vertex_ast = nil
    if shader_def.vertex then
        local ctx, env = create_recorder(shader_def.uniforms)
        active_env = env -- Activate Recorder Env
        
        -- Execute
        shader_def.vertex() 
        
        active_env = nil -- Deactivate
        
        vertex_ast = ctx.get_block()
        merge_reflection(ctx.get_reflection_data())
    end
    
    local fragment_ast = nil
    if shader_def.fragment then
        local ctx, env = create_recorder(shader_def.uniforms)
        active_env = env
        
        shader_def.fragment()
        
        active_env = nil
        
        fragment_ast = ctx.get_block()
        merge_reflection(ctx.get_reflection_data())
    end
    
    -- Finalize Definitions
    local function finalize_table(explicit, reflected, default_type)
        local out = explicit or {}
        for k,_ in pairs(reflected) do
            if not out[k] then
                out[k] = default_type or "vec4" 
            end
        end
        return out
    end

    return {
        name = shader_def.name,
        vertex_block = vertex_ast,
        fragment_block = fragment_ast,
        uniforms = finalize_table(shader_def.uniforms, reflection.uniforms, "vec4"),
        attributes = finalize_table(shader_def.attributes, reflection.attributes, "vec4"),
        varyings = finalize_table(shader_def.varyings, reflection.varyings, "vec4")
    }
end

function ZShader.transpile(ast, backend)
    if backend == "glsl120" then
        return require("zshader.transpilers.glsl").transpile(ast)
    end
    error("Unknown backend")
end

return ZShader
