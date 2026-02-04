
local AST = {}

local mt = {
    __index = function(t, k)
        -- Array Access
        if type(k) == "number" then
            return AST.make_node("array_access", { array = t, index = k })
        end
        
        -- String Keys
        if type(k) == "string" then
            -- Swizzling
            if k:match("^[xyzw01rgba]+$") then
                return AST.make_node("swizzle", { vec = t, components = k })
            end
            
            -- Explicit Comparison/Methods
            if k == "gt" then return function(self, o) return AST.binary_op(">", self, o) end end
            if k == "lt" then return function(self, o) return AST.binary_op("<", self, o) end end
            if k == "ge" then return function(self, o) return AST.binary_op(">=", self, o) end end
            if k == "le" then return function(self, o) return AST.binary_op("<=", self, o) end end
            if k == "eq" then return function(self, o) return AST.binary_op("==", self, o) end end
            if k == "ne" then return function(self, o) return AST.binary_op("!=", self, o) end end
            
            -- Generic Member Access (e.g. struct fields)
            return AST.make_node("member_access", { object = t, member = k })
        end
    end,
    
    __add = function(a, b) return AST.binary_op("+", a, b) end,
    __sub = function(a, b) return AST.binary_op("-", a, b) end,
    __mul = function(a, b) return AST.binary_op("*", a, b) end,
    __div = function(a, b) return AST.binary_op("/", a, b) end,
    __mod = function(a, b) return AST.binary_op("%", a, b) end,
    __unm = function(a) return AST.make_node("unary_op", { op = "-", operand = a }) end,
    
    -- Best effort Lua operators (debugging/logic) 
    __tostring = function(t) 
        local override = rawget(t, "override_tostring")
        if override then return override(t) end
        return "AST(" .. t.type .. ")" 
    end
}

function AST.make_node(type, data)
    data.type = type
    setmetatable(data, mt)
    return data
end

function AST.binary_op(op, a, b)
    return AST.make_node("binary_op", { op = op, left = a, right = b })
end

return AST
