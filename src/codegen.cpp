#include "codegen.hpp"
#include <fstream>
#include <cstdlib>
#include <cctype>

static bool is_int_literal(const std::string &s)
{
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    if (i >= s.size()) return false;
    for (; i < s.size(); ++i) if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}

static std::string op_to_asm(const std::string &op)
{
    if (op == "+") return "add";
    if (op == "-") return "sub";
    if (op == "*") return "imul";
    return "";
}

static std::string cmp_to_jmp(const std::string &c)
{
    if (c == "<")  return "jl";
    if (c == "<=") return "jle";
    if (c == ">")  return "jg";
    if (c == ">=") return "jge";
    if (c == "==") return "je";
    if (c == "!=") return "jne";
    return "";
}

CodeGenerator::CodeGenerator(const InterCodeArray &arr,
                             const std::unordered_map<std::string, std::string> &identifiers,
                             const std::unordered_map<std::string, std::string> &constants,
                             const std::unordered_map<std::string, std::string> &tempmap)
    : arr(arr), ids(identifiers), consts(constants), tempmap(tempmap) {}

void CodeGenerator::pr(const std::string &s)
{
    out.append(s);
    out.push_back('\n');
}

std::string CodeGenerator::handleVar(const std::string &a,
                                     const std::unordered_map<std::string, std::string> &tempmap)
{
    auto it = tempmap.find(a);
    if (it != tempmap.end()) return it->second;
    return a;
}

void CodeGenerator::gen_variables()
{
    pr("section .bss");
    if (need_print_num)
    {
        pr("\tdigitSpace resb 128");
        pr("\tdigitSpacePos resq 1");
        pr("");
    }

    for (auto &kv : ids)
        pr("\t" + kv.first + " resq 1");

    for (auto &kv : tempmap)
        pr("\t" + kv.second + " resq 1");
}

void CodeGenerator::gen_start()
{
    pr("section .data");
    for (auto &kv : consts)
    {

        pr("\t" + kv.first + " db \"" + kv.second + "\",10");
        pr("\t" + kv.first + "_len equ $-" + kv.first);
    }
    pr("");
    pr("section .text");
    pr("\tglobal _start");
    pr("");
    pr("_start:");
}

void CodeGenerator::gen_assignment(const AssignmentCode &a)
{
    const auto dst = handleVar(a.var, tempmap);

    if (a.op.empty())
    {
            if (is_int_literal(a.left)) pr("\tmov rax, " + a.left);
    else pr("\tmov rax, qword [" + handleVar(a.left, tempmap) + "]");
        pr("\tmov qword [" + dst + "], rax");
        return;
    }

        if (is_int_literal(a.left)) pr("\tmov rax, " + a.left);
    else pr("\tmov rax, qword [" + handleVar(a.left, tempmap) + "]");

    if (a.op == "/" || a.op == "%")
    {
            if (is_int_literal(a.right)) pr("\tmov rbx, " + a.right);
    else pr("\tmov rbx, qword [" + handleVar(a.right, tempmap) + "]");
        pr("\tcqo");
        pr("\tidiv rbx");
        if (a.op == "%")
            pr("\tmov qword [" + dst + "], rdx");
        else
            pr("\tmov qword [" + dst + "], rax");
        return;
    }

    const auto ins = op_to_asm(a.op);
        if (is_int_literal(a.right)) pr("\tmov rbx, " + a.right);
    else pr("\tmov rbx, qword [" + handleVar(a.right, tempmap) + "]");
    if (ins.empty())
    {
        pr("\t; unsupported op '" + a.op + "'");
        pr("\tmov qword [" + dst + "], rax");
        return;
    }
    pr("\t" + ins + " rax, rbx");
    pr("\tmov qword [" + dst + "], rax");
}

void CodeGenerator::gen_jump(const JumpCode &j)
{
    pr("\tjmp " + j.dist);
}

void CodeGenerator::gen_label(const LabelCode &l)
{
    pr(l.label + ":");
}

void CodeGenerator::gen_compare(const CompareCodeIR &c)
{
    const auto jmp = cmp_to_jmp(c.operation);
    if (jmp.empty())
    {
        pr("\t; unsupported compare '" + c.operation + "'");
        return;
    }

        if (is_int_literal(c.left)) pr("\tmov rax, " + c.left);
    else pr("\tmov rax, qword [" + handleVar(c.left, tempmap) + "]");
        if (is_int_literal(c.right)) pr("\tmov rbx, " + c.right);
    else pr("\tmov rbx, qword [" + handleVar(c.right, tempmap) + "]");
    pr("\tcmp rax, rbx");
    pr("\t" + jmp + " " + c.jump);
}

void CodeGenerator::gen_print(const PrintCodeIR &p)
{
    if (p.type == "string")
    {
        need_print_string = true;
        pr("\tmov rsi, " + p.value);
        pr("\tmov rdx, " + p.value + "_len");
        pr("\tcall print_string");
        return;
    }

    need_print_num = true;
    if (is_int_literal(p.value))
        pr("\tmov rdi, " + p.value);
    else
        pr("\tmov rdi, qword [" + handleVar(p.value, tempmap) + "]");
    pr("\tcall print_num");
}

void CodeGenerator::gen_code()
{
    for (auto &ins : arr.code)
    {
        switch (ins->kind())
        {
        case IRKind::Assignment:
            gen_assignment(*std::static_pointer_cast<AssignmentCode>(ins));
            break;
        case IRKind::Jump:
            gen_jump(*std::static_pointer_cast<JumpCode>(ins));
            break;
        case IRKind::Label:
            gen_label(*std::static_pointer_cast<LabelCode>(ins));
            break;
        case IRKind::Compare:
            gen_compare(*std::static_pointer_cast<CompareCodeIR>(ins));
            break;
        case IRKind::Print:
            gen_print(*std::static_pointer_cast<PrintCodeIR>(ins));
            break;
        }
    }
}

void CodeGenerator::gen_end()
{
    pr("");
    pr("\tmov rax, 60"); // exit
    pr("\txor rdi, rdi");
    pr("\tsyscall");
}

void CodeGenerator::gen_print_string_function()
{
    pr("");
    pr("print_string:");
    pr("\tmov rax, 1");
    pr("\tmov rdi, 1");
    pr("\tsyscall");
    pr("\tret");
}

void CodeGenerator::gen_print_num_function()
{
    pr("");
    pr("print_num:");
    pr("\tpush rbx");
    pr("\tpush rcx");
    pr("\tpush rdx");
    pr("\tpush r8");
    pr("\tpush r9");
    pr("\tpush r10");
    pr("\tpush r11");
    pr("\tmov rax, rdi");
    pr("\txor r8, r8");
    pr("\tcmp rax, 0");
    pr("\tjge .pn_abs");
    pr("\tneg rax");
    pr("\tmov r8, 1");
    pr(".pn_abs:");
    pr("\tmov rcx, digitSpace");
    pr("\tmov byte [rcx], 10");
    pr("\tinc rcx");
    pr("\tmov qword [digitSpacePos], rcx");
    pr("\tcmp rax, 0");
    pr("\tjne .pn_loop");
    pr("\tmov rcx, qword [digitSpacePos]");
    pr("\tmov byte [rcx], '0'");
    pr("\tinc rcx");
    pr("\tmov qword [digitSpacePos], rcx");
    pr("\tjmp .pn_done");
    pr(".pn_loop:");
    pr("\txor rdx, rdx");
    pr("\tmov rbx, 10");
    pr("\tdiv rbx");
    pr("\tadd dl, '0'");
    pr("\tmov rcx, qword [digitSpacePos]");
    pr("\tmov byte [rcx], dl");
    pr("\tinc rcx");
    pr("\tmov qword [digitSpacePos], rcx");
    pr("\tcmp rax, 0");
    pr("\tjne .pn_loop");
    pr(".pn_done:");
    pr("\tcmp r8, 0");
    pr("\tje .pn_print");
    pr("\tmov rax, 1");
    pr("\tmov rdi, 1");
    pr("\tlea rsi, [rel .pn_minus]");
    pr("\tmov rdx, 1");
    pr("\tsyscall");
    pr(".pn_print:");
    pr("\tmov rcx, qword [digitSpacePos]");
    pr(".pn_print_loop:");
    pr("\tdec rcx");
    pr("\tmov rax, 1");
    pr("\tmov rdi, 1");
    pr("\tmov rsi, rcx");
    pr("\tmov rdx, 1");
    pr("\tsyscall");
    pr("\tcmp rcx, digitSpace");
    pr("\tjne .pn_print_loop");
    pr("\tpop r11");
    pr("\tpop r10");
    pr("\tpop r9");
    pr("\tpop r8");
    pr("\tpop rdx");
    pr("\tpop rcx");
    pr("\tpop rbx");
    pr("\tret");
    pr(".pn_minus: db '-'");
}

void CodeGenerator::writeAsm(const std::string &path)
{
    out.clear();

    for (auto &ins : arr.code)
    {
        if (ins->kind() == IRKind::Print)
        {
            auto print_ins = std::static_pointer_cast<PrintCodeIR>(ins);
            if (print_ins->type == "string")
                need_print_string = true;
            else
                need_print_num = true;
        }
    }

    gen_variables();
    gen_start();
    gen_code();
    gen_end();
    if (need_print_string)
        gen_print_string_function();
    if (need_print_num)
        gen_print_num_function();

    std::ofstream f(path, std::ios::binary);
    f << out;
    f.close();
}

int CodeGenerator::assembleAndRun(const std::string &asmPath,
                                 const std::string &objPath,
                                 const std::string &exePath)
{
    std::string cmd1 = "nasm -f elf64 " + asmPath + " -o " + objPath;
    std::string cmd2 = "ld " + objPath + " -o " + exePath;
    std::string cmd3 = "./" + exePath;
    int r1 = std::system(cmd1.c_str());
    if (r1 != 0) return r1;
    int r2 = std::system(cmd2.c_str());
    if (r2 != 0) return r2;
    return std::system(cmd3.c_str());
}
