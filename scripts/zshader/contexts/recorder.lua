
local AST = require("zshader.ast")

local function create_recorder_context(declared_uniforms)
    local ctx = {}
    
    -- State
    local statements = {}
    local stack = { statements }
    
    local function push_stmt(stmt)
        table.insert(stack[#stack], stmt)
    end
    
    ctx.get_block = function() return { statements = statements } end

    -- REFLECTION STATE
    local active_inputs = {
        uniform = {},
        attribute = {},
        varying = {}
    }

    function ctx.get_reflection_data()
        return active_inputs
    end
    
    -- ENVIRONMENT TABLE
    local env = {}
    
    local BUILTINS = {
        LightSource = true,
        FrontMaterial = true,
        LightModel = true,
        ModelViewMatrix = true,
        ProjectionMatrix = true,
        TextureMatrix = true,
        NormalMatrix = true
    }

    -- PROXIES FOR TYPES
    local function make_ctor(name, args)
        local arg_nodes = {}
        for i,v in ipairs(args) do table.insert(arg_nodes, v) end
        return AST.make_node("call", { func = name, args = arg_nodes })
    end
    
    function env.vec2(...) return make_ctor("vec2", {...}) end
    function env.vec3(...) return make_ctor("vec3", {...}) end
    function env.vec4(...) return make_ctor("vec4", {...}) end
    function env.mat4(...) return make_ctor("mat4", {...}) end
    
    -- MATH
    function env.sin(x) return AST.make_node("call", { func = "sin", args = {x} }) end
    function env.cos(x) return AST.make_node("call", { func = "cos", args = {x} }) end
    function env.float(x) return AST.make_node("call", { func = "float", args = {x} }) end
    function env.dot(x,y) return AST.make_node("call", { func = "dot", args = {x,y} }) end
    function env.mix(a, b, t) return AST.make_node("call", { func = "mix", args = {a, b, t} }) end
    function env.normalize(v) return AST.make_node("call", { func = "normalize", args = {v} }) end
    function env.pow(b, e) return AST.make_node("call", { func = "pow", args = {b, e} }) end
    function env.max(a, b) return AST.make_node("call", { func = "max", args = {a, b} }) end
    function env.min(a, b) return AST.make_node("call", { func = "min", args = {a, b} }) end
    function env.clamp(v, min, max) return AST.make_node("call", { func = "clamp", args = {v, min, max} }) end
    function env.length(v) return AST.make_node("call", { func = "length", args = {v} }) end
    function env.distance(p1, p2) return AST.make_node("call", { func = "distance", args = {p1, p2} }) end
    function env.reflect(i, n) return AST.make_node("call", { func = "reflect", args = {i, n} }) end
    function env.step(edge, x) return AST.make_node("call", { func = "step", args = {edge, x} }) end
    function env.smoothstep(e0, e1, x) return AST.make_node("call", { func = "smoothstep", args = {e0, e1, x} }) end
    function env.mul(a, b) return AST.make_node("binary_op", { op = "*", left = a, right = b }) end
    
    -- TEXTURE
    function env.texture_sample(tex, uv) 
        return AST.make_node("call", { func = "texture_sample", args = {tex, uv} }) 
    end

    -- ABSTRACTIONS (Input/Output/Varying)
    local Input = {
        Position = function() 
            active_inputs.attribute["__POSITION__"] = "vec4"
            return AST.make_node("symbol", { name = "__POSITION__" }) 
        end,
        Normal = function() 
            active_inputs.attribute["__NORMAL__"] = "vec3"
            return AST.make_node("symbol", { name = "__NORMAL__" }) 
        end,
        Color = function() 
            active_inputs.attribute["__COLOR__"] = "vec4"
            return AST.make_node("symbol", { name = "__COLOR__" }) 
        end,
        UV = function() 
            active_inputs.attribute["__UV__"] = "vec4"
            return AST.make_node("symbol", { name = "__UV__" }) 
        end
    }
    env.Input = Input

    local Matrix = {
        MVP = function() 
            active_inputs.uniform["__MVP__"] = "mat4"
            return AST.make_node("symbol", { name = "__MVP__" }) 
        end,
        Normal = function()
            active_inputs.uniform["__NORMAL_MATRIX__"] = "mat3"
            return AST.make_node("symbol", { name = "__NORMAL_MATRIX__" })
        end
    }
    env.Matrix = Matrix
    
    local Output = {}
    setmetatable(Output, {
        __newindex = function(t, k, v)
            if k == "Position" then
                push_stmt({ type = "assignment", target = "__OUTPUT_POSITION__", expr = v })
            elseif k == "Color" then
                push_stmt({ type = "assignment", target = "__OUTPUT_COLOR__", expr = v })
            else
                -- Ignore or Error
            end
        end
    })
    env.Output = Output

    local Varying = {}
    setmetatable(Varying, {
        __index = function(t, k)
            active_inputs.varying[k] = true
            return AST.make_node("symbol", { name = k })
        end,
        __newindex = function(t, k, v)
             push_stmt({ type = "assignment", target = k, expr = v })
             active_inputs.varying[k] = true
        end
    })
    env.Varying = Varying

    -- UNIFORMS PROXY
    local uniforms_proxy = {}
    setmetatable(uniforms_proxy, {
        __index = function(t, k)
             if BUILTINS[k] or k:find("^gl_") then
                 return AST.make_node("symbol", { name = k })
             end
             
             if declared_uniforms then
                 if not declared_uniforms[k] then
                     error("Undeclared uniform accessed: " .. k)
                 end
             end
             
             active_inputs.uniform[k] = true
             return AST.make_node("symbol", { name = k })
        end
    })
    env.uniforms = uniforms_proxy
    
    function env.eq(a, b) return AST.make_node("binary_op", { op = "==", left = a, right = b }) end
    function env.ne(a, b) return AST.make_node("binary_op", { op = "!=", left = a, right = b }) end
    function env.lt(a, b) return AST.make_node("binary_op", { op = "<", left = a, right = b }) end
    function env.gt(a, b) return AST.make_node("binary_op", { op = ">", left = a, right = b }) end
    function env.le(a, b) return AST.make_node("binary_op", { op = "<=", left = a, right = b }) end
    function env.ge(a, b) return AST.make_node("binary_op", { op = ">=", left = a, right = b }) end

    -- VARIABLES and MUTABILITY
    function env.Var(name, type, init)
        push_stmt({ type = "declaration", name = name, vartype = type, expr = init })
        return AST.make_node("symbol", { name = name })
    end
    
    function env.Assign(target, expr)
        local name = target
        if type(target) == "table" and target.type == "symbol" then
            name = target.name
        end
        push_stmt({ type = "assignment", target = name, expr = expr })
    end

    -- CONTROL FLOW
    function env.If(cond, then_fn)
        local if_stmt = { type = "if", condition = cond, then_block = nil, else_block = nil }
        push_stmt(if_stmt)
        
        -- Capture Then Context
        local then_stmts = {}
        table.insert(stack, then_stmts)
        then_fn() 
        table.remove(stack)
        if_stmt.then_block = { statements = then_stmts }
        
        return {
            Else = function(else_fn)
                local else_stmts = {}
                table.insert(stack, else_stmts)
                else_fn()
                table.remove(stack)
                if_stmt.else_block = { statements = else_stmts }
            end
        }
    end

    -- GLOBAL ENV MAGIC
    local env_mt = {}
    env_mt.__index = function(t, k)
        if _G[k] then return _G[k] end
        return nil
    end
    
    env_mt.__newindex = function(t, k, v)
         push_stmt({ type = "assignment", target = k, expr = v })
    end
    
    setmetatable(env, env_mt)

    return ctx, env
end

return create_recorder_context
