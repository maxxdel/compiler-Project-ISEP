#include "ir.hpp"
#include <stdexcept>

static std::shared_ptr<AssignmentCode> make_assign(const std::string &v, const std::string &l,
                                                   const std::string &op, const std::string &r)
{
    auto a = std::make_shared<AssignmentCode>();
    a->var = v;
    a->left = l;
    a->op = op;
    a->right = r;
    return a;
}
static std::shared_ptr<JumpCode> make_jump(const std::string &d)
{
    auto j = std::make_shared<JumpCode>();
    j->dist = d;
    return j;
}
static std::shared_ptr<LabelCode> make_label(const std::string &l)
{
    auto x = std::make_shared<LabelCode>();
    x->label = l;
    return x;
}
static std::shared_ptr<CompareCodeIR> make_compare(const std::string &l, const std::string &op,
                                                   const std::string &r, const std::string &j)
{
    auto c = std::make_shared<CompareCodeIR>();
    c->left = l;
    c->operation = op;
    c->right = r;
    c->jump = j;
    return c;
}
static std::shared_ptr<PrintCodeIR> make_print(const std::string &t, const std::string &v)
{
    auto p = std::make_shared<PrintCodeIR>();
    p->type = t;
    p->value = v;
    return p;
}

IntermediateCodeGen::IntermediateCodeGen(const std::shared_ptr<Node> &root) : root(root)
{
    exec_statement(root);
}

std::string IntermediateCodeGen::nextTemp()
{
    std::string t = "T" + std::to_string(tCounter++);
    std::string sym = "__tmp" + std::to_string(tCounter - 1);
    tempmap[t] = sym;
    return t;
}

std::string IntermediateCodeGen::nextLabel() { return "L" + std::to_string(lCounter++); }
std::string IntermediateCodeGen::nextStringSym() { return "S" + std::to_string(sCounter++); }

static bool is_arith_op(const std::string &op)
{
    return op == "+" || op == "-" || op == "*" || op == "/";
}
static bool is_cmp_op(const std::string &op)
{
    return op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=";
}

std::string IntermediateCodeGen::exec_expr(const std::shared_ptr<Node> &n)
{
    if (!n)
        throw std::runtime_error("IR: null expression");

    if (auto id = std::dynamic_pointer_cast<IdentifierNode>(n))
        return id->getValue();

    if (auto num = std::dynamic_pointer_cast<NumberNode>(n))
        return num->getValue();

    if (auto un = std::dynamic_pointer_cast<UnaryOpNode>(n))
    {
        throw std::runtime_error("IR: unary operator used as value expression: " + un->op_tok.value);
    }

    auto bin = std::dynamic_pointer_cast<BinOpNode>(n);
    if (!bin)
        throw std::runtime_error("IR: unsupported expression node");

    if (!is_arith_op(bin->op_tok.value))
    {
        throw std::runtime_error("IR: non-arithmetic operator used as value expression: " + bin->op_tok.value);
    }

    auto left = exec_expr(bin->left);
    auto right = exec_expr(bin->right);

    auto t = nextTemp();
    arr.append(make_assign(t, left, bin->op_tok.value, right));
    return t;
}

void IntermediateCodeGen::emit_condition(const std::shared_ptr<Node> &cond,
                                         const std::string &trueLabel,
                                         const std::string &falseLabel)
{
    if (!cond)
        throw std::runtime_error("IR: null condition");

    if (auto un = std::dynamic_pointer_cast<UnaryOpNode>(cond))
    {
        if (un->op_tok.value != "!")
            throw std::runtime_error("IR: unsupported unary condition op: " + un->op_tok.value);
        emit_condition(un->operand, falseLabel, trueLabel);
        return;
    }
    if (auto bin = std::dynamic_pointer_cast<BinOpNode>(cond))
    {
        const std::string op = bin->op_tok.value;

        if (op == "!" && !bin->left)
        {
            emit_condition(bin->right, falseLabel, trueLabel);
            return;
        }

        if (op == "&&")
        {
            auto mid = nextLabel();
            emit_condition(bin->left, mid, falseLabel);
            arr.append(make_label(mid));
            emit_condition(bin->right, trueLabel, falseLabel);
            return;
        }
        if (op == "||")
        {
            auto mid = nextLabel();
            emit_condition(bin->left, trueLabel, mid);
            arr.append(make_label(mid));
            emit_condition(bin->right, trueLabel, falseLabel);
            return;
        }
        if (is_cmp_op(op))
        {
            auto left = exec_expr(bin->left);
            auto right = exec_expr(bin->right);
            arr.append(make_compare(left, op, right, trueLabel));
            arr.append(make_jump(falseLabel));
            return;
        }
    }

    auto v = exec_expr(cond);
    arr.append(make_compare(v, "!=", "0", trueLabel));
    arr.append(make_jump(falseLabel));
}

void IntermediateCodeGen::exec_assignment(const std::shared_ptr<AssignmentNode> &a)
{
    auto right = exec_expr(a->expression);
    arr.append(make_assign(a->identifier.value, right, "", ""));
}

void IntermediateCodeGen::exec_print(const std::shared_ptr<PrintNode> &p)
{
    if (!p || !p->value)
        return;

    if (auto lit = std::dynamic_pointer_cast<NumberNode>(p->value))
    {
        if (lit->tok.type == TokenType::String)
        {
            auto sym = nextStringSym();
            constants[sym] = lit->tok.value;
            arr.append(make_print("string", sym));
            return;
        }
    }

    auto v = exec_expr(p->value);
    arr.append(make_print("int", v));
}

void IntermediateCodeGen::exec_declaration(const std::shared_ptr<DeclarationNode> &d)
{
    if (!d)
        return;
    identifiers[d->identifier.value] = (d->var_type == ValueType::Int) ? "int" : "string";
}

void IntermediateCodeGen::exec_block(const std::shared_ptr<BlockNode> &b)
{
    if (!b)
        return;
    for (const auto &st : b->statements)
        exec_statement(st);
}

void IntermediateCodeGen::exec_if(const std::shared_ptr<IfNode> &i)
{
    auto thenL = nextLabel();
    auto endL = nextLabel();

    if (i->else_branch)
    {
        auto elseL = nextLabel();
        emit_condition(i->condition, thenL, elseL);

        arr.append(make_label(thenL));
        exec_statement(i->then_branch);
        arr.append(make_jump(endL));

        arr.append(make_label(elseL));
        exec_statement(i->else_branch);

        arr.append(make_label(endL));
    }
    else
    {
        emit_condition(i->condition, thenL, endL);

        arr.append(make_label(thenL));
        exec_statement(i->then_branch);

        arr.append(make_label(endL));
    }
}

void IntermediateCodeGen::exec_while(const std::shared_ptr<WhileNode> &w)
{
    auto startL = nextLabel();
    auto bodyL = nextLabel();
    auto endL = nextLabel();

    arr.append(make_label(startL));
    emit_condition(w->condition, bodyL, endL);

    arr.append(make_label(bodyL));
    exec_statement(w->body);
    arr.append(make_jump(startL));

    arr.append(make_label(endL));
}

void IntermediateCodeGen::exec_statement(const std::shared_ptr<Node> &n)
{
    if (!n)
        return;

    if (auto blk = std::dynamic_pointer_cast<BlockNode>(n))
    {
        exec_block(blk);
        return;
    }
    if (auto iff = std::dynamic_pointer_cast<IfNode>(n))
    {
        exec_if(iff);
        return;
    }
    if (auto wh = std::dynamic_pointer_cast<WhileNode>(n))
    {
        exec_while(wh);
        return;
    }
    if (auto pr = std::dynamic_pointer_cast<PrintNode>(n))
    {
        exec_print(pr);
        return;
    }
    if (auto de = std::dynamic_pointer_cast<DeclarationNode>(n))
    {
        exec_declaration(de);
        return;
    }
    if (auto asg = std::dynamic_pointer_cast<AssignmentNode>(n))
    {
        exec_assignment(asg);
        return;
    }

    throw std::runtime_error("IR: unsupported statement node");
}
