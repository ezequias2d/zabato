
local Transpiler = {}

local function map_type(t)
    if t == "Texture2D" then return "sampler2D" end
    return t
end

local SYMBOL_MAP = {
    __POSITION__ = "gl_Vertex",
    __NORMAL__ = "gl_Normal",
    __COLOR__ = "gl_Color",
    __UV__ = "gl_MultiTexCoord0",
    __MVP__ = "gl_ModelViewProjectionMatrix",
    __OUTPUT_POSITION__ = "gl_Position",
    __OUTPUT_COLOR__ = "gl_FragColor",
    __NORMAL_MATRIX__ = "gl_NormalMatrix",
    -- Abstractions
    LightSource = "gl_LightSource",
    FrontMaterial = "gl_FrontMaterial",
    LightModel = "gl_LightModel",
    ModelViewMatrix = "gl_ModelViewMatrix"
}

local FUNCTION_MAP = {
    texture_sample = "texture2D"
}

local function print_expr(node)
    if type(node) ~= "table" then return tostring(node) end
    if node.type == "binary_op" then
        return "(" .. print_expr(node.left) .. " " .. node.op .. " " .. print_expr(node.right) .. ")"
    elseif node.type == "call" then
        local args = {}
        for i,a in ipairs(node.args) do table.insert(args, print_expr(a)) end
        local func = FUNCTION_MAP[node.func] or node.func
        return func .. "(" .. table.concat(args, ", ") .. ")"
    elseif node.type == "symbol" then
        return SYMBOL_MAP[node.name] or node.name
    elseif node.type == "array_access" then
        return print_expr(node.array) .. "[" .. print_expr(node.index) .. "]"
    elseif node.type == "member_access" then
        return print_expr(node.object) .. "." .. node.member
    elseif node.type == "unary_op" then
        return node.op .. print_expr(node.operand)
    elseif node.type == "swizzle" then
        return print_expr(node.vec) .. "." .. node.components
    end
    return tostring(node)
end

local function print_stmt(stmt, indent)
    local pre = string.rep("  ", indent)
    if stmt.type == "assignment" then
        local target = SYMBOL_MAP[stmt.target] or stmt.target
        return pre .. target .. " = " .. print_expr(stmt.expr) .. ";"
    elseif stmt.type == "declaration" then
        return pre .. stmt.vartype .. " " .. stmt.name .. " = " .. print_expr(stmt.expr) .. ";"
    elseif stmt.type == "if" then
        local s = pre .. "if (" .. print_expr(stmt.condition) .. ") {\n"
        for _, sub in ipairs(stmt.then_block.statements) do
            s = s .. print_stmt(sub, indent + 1) .. "\n"
        end
        s = s .. pre .. "}"
        if stmt.else_block then
            s = s .. " else {\n"
            for _, sub in ipairs(stmt.else_block.statements) do
                s = s .. print_stmt(sub, indent + 1) .. "\n"
            end
            s = s .. pre .. "}"
        end
        return s
    end
    return pre .. "// Unknown stmt"
end

function Transpiler.transpile(ast)
    local lines = {}
    table.insert(lines, "#version 120")
    
    local function generate_decls(ast, allow_attributes)
        local d = {}
        for k,v in pairs(ast.uniforms or {}) do 
            local map_name = SYMBOL_MAP[k]
            if not k:find("^gl_") and not map_name then 
                local type_name = type(v) == "table" and v.type or v
                table.insert(d, "uniform " .. map_type(type_name) .. " " .. k .. ";") 
            end
        end
        if allow_attributes then
             for k,v in pairs(ast.attributes or {}) do 
                 local map_name = SYMBOL_MAP[k]
                 if not k:find("^gl_") and not map_name then 
                     table.insert(d, "attribute " .. map_type(v) .. " " .. k .. ";") 
                 end
             end
        end
        for k,v in pairs(ast.varyings or {}) do table.insert(d, "varying " .. map_type(v) .. " " .. k .. ";") end
        return table.concat(d, "\n")
    end

    table.insert(lines, generate_decls(ast, true))
    
    if ast.vertex_block then
         table.insert(lines, "void main() {")
         for _, s in ipairs(ast.vertex_block.statements) do
             table.insert(lines, print_stmt(s, 1))
         end
         table.insert(lines, "}")
    end

    if ast.fragment_block then
         table.insert(lines, "// --- FRAGMENT ---")
         table.insert(lines, generate_decls(ast, false))
         table.insert(lines, "void main() {")
         for _, s in ipairs(ast.fragment_block.statements) do
             table.insert(lines, print_stmt(s, 1))
         end
         table.insert(lines, "}")
    end
    
    return table.concat(lines, "\n")
end

return Transpiler
