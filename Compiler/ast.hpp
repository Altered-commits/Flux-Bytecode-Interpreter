/* ast.hpp is responsible for few things,
 * 1) Defining nodes for different expressions ofc
 * 2) Performing a sort of semantic analysis as well
 * example: can left expr can be added to right expr? if yes what is the resultant type
*/
#ifndef UNNAMED_AST_HPP
#define UNNAMED_AST_HPP

#include <memory>
#include <string>
#include "..\Common\common.hpp"
#include "error_printer.hpp"
#include "type_checker.hpp"

//Forward declare cuz the below std::unique_ptr is going to cry if not done
struct ASTNode;

using ASTPtr    = std::unique_ptr<ASTNode>;
using ASTRawPtr = ASTNode*;

//This thing will at max work for enum values under 64, cuz std::size_t aint 128bit
constexpr std::size_t comparisionTypeMask = (1ULL << static_cast<std::size_t>(TOKEN_EEQ))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_NEQ))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_GT))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_LT))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_GTEQ))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_LTEQ));

constexpr std::size_t termExprTypeMask = (1ULL << static_cast<std::size_t>(TOKEN_DIV))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_MULT))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_MODULO));

//Used for comparing token type in a type mask
#define SAME_PRECEDENCE_MASK_CHECK(token, mask) (((1ULL << static_cast<std::size_t>(token)) & mask))

//Some enums for Compile time evaluation
enum class CTType {
    NA,
    Value,
    Binary,
    Unary,
    Cast,
    VarAccess
};

//For the visitor pattern, we need to forward declare all the nodes, here we go
struct ASTValue;
struct ASTBinaryOp;
struct ASTUnaryOp;
struct ASTVariableAssign;
struct ASTVariableAccess;
struct ASTCastNode;
struct ASTBlock;
struct ASTTernaryOp;
struct ASTIfNode;
struct ASTRangeIterator;
struct ASTForNode;
struct ASTWhileNode;
struct ASTContinue;
struct ASTBreak;

//We will be using the -> Visitor Pattern
//--------Visitor interface--------
//The bool is there to tell if something is a sub expression, this is useful for many things such as
//Multiple variable assignment: so that when something is assigned to variable, it doesnt pop the stack to get value
class ASTVisitorInterface
{
    public:
        virtual void visit(ASTValue& node, bool)          = 0;
        virtual void visit(ASTBinaryOp& node, bool)       = 0;
        virtual void visit(ASTUnaryOp& node, bool)        = 0;
        virtual void visit(ASTVariableAssign& node, bool) = 0;
        virtual void visit(ASTVariableAccess& node, bool) = 0;
        virtual void visit(ASTCastNode& node, bool)       = 0;
        virtual void visit(ASTBlock& node, bool)          = 0;
        virtual void visit(ASTTernaryOp& node, bool)      = 0;
        virtual void visit(ASTIfNode& node, bool)         = 0;
        virtual void visit(ASTRangeIterator& node, bool)  = 0;
        virtual void visit(ASTForNode& node, bool)        = 0;
        virtual void visit(ASTWhileNode& node, bool)      = 0;
        virtual void visit(ASTContinue& node, bool)       = 0;
        virtual void visit(ASTBreak& node, bool)          = 0;
};

//AST nodes
struct ASTNode
{
    virtual ~ASTNode() = default;

    //Visitor
    virtual void accept(ASTVisitorInterface& visitor, bool is_sub_expr) = 0;

    //AST Checks
    virtual bool isFloat() const { return false; }
    virtual bool isPowerOp() const { return false; }

    //Think of it as a tag for each type
    virtual CTType getType() const { return CTType::NA; }

    //Expression evaluation for final type (variables and range)
    virtual EvalType evaluateExprType() const { return EVAL_UNKNOWN; }
    virtual EvalType evaluateIterType() const { return EVAL_UNKNOWN; }
};

//-----------------------------AST Types-----------------------------
struct ASTValue : public ASTNode
{
    EvalType    type;
    std::string value;

    ASTValue(EvalType type, const std::string& value)
        : type(type), value(value)
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    bool isFloat() const override {
        return type == EVAL_FLOAT;
    }
    bool isPowerOp() const override {
        return false;
    }

    EvalType evaluateExprType() const override {
        return type;
    }

    //Tag
    CTType getType() const override {
        return CTType::Value;
    }
};

struct ASTBinaryOp : public ASTNode
{
    TokenType op_type;
    ASTPtr left;
    ASTPtr right;

    ASTBinaryOp(TokenType op_type, ASTPtr&& left, ASTPtr&& right)
        : op_type(op_type), left(std::move(left)), right(std::move(right))
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    bool isFloat() const override {
        return left->isFloat() || right->isFloat();
    }
    bool isPowerOp() const override {
        return op_type == TOKEN_POW || left->isPowerOp() || right->isPowerOp();
    }

    EvalType evaluateExprType() const override {
        auto& evaluator       = typeCheckerForCommonOps.at(op_type);
        const auto& eval_type = evaluator.find(std::make_pair(left->evaluateExprType(), right->evaluateExprType()));

        if(eval_type == evaluator.end())
            printError("ASTError -> Binary Operation", "No operator overload defined for the expression on left and right of operator");
        
        return eval_type->second;
    }

    //Tag
    CTType getType() const override {
        return CTType::Binary;
    }
};

struct ASTUnaryOp : public ASTNode
{
    TokenType op_type;
    ASTPtr    expr;

    ASTUnaryOp(TokenType op_type, ASTPtr&& expr)
        : op_type(op_type), expr(std::move(expr))
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    bool isFloat() const override {
        return expr->isFloat();
    }
    bool isPowerOp() const override {
        return expr->isPowerOp();
    }

    EvalType evaluateExprType() const override {
        auto& evaluator       = typeCheckerForUnaryOps.at(op_type);
        const auto& eval_type = evaluator.find(expr->evaluateExprType());

        if(eval_type == evaluator.end())
            printError("ASTError -> Unary Operation", "No operator overload defined for the expression given to Unary Operator");
        
        return eval_type->second;
    }

    //Tag
    CTType getType() const override {
        return CTType::Unary;
    }
};

struct ASTVariableAssign : public ASTNode
{
    std::string  identifier;
    ASTPtr       expr;
    EvalType     var_type;
    bool         is_reassignment;
    //Same here as VariableAccess
    std::uint16_t scope_index;

    ASTVariableAssign(const std::string& identifier, EvalType type, ASTPtr&& expr, bool is_reassignment, std::uint16_t scope_index)
        : identifier(identifier), var_type(type), expr(std::move(expr)), is_reassignment(is_reassignment), scope_index(scope_index)
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    bool isFloat() const override {
        return var_type == EVAL_FLOAT;
    }
    bool isPowerOp() const override {
        return expr->isPowerOp();
    }

    EvalType evaluateExprType() const override {
        return var_type;
    }
};

struct ASTVariableAccess : public ASTNode
{
    std::string identifier;
    EvalType    var_type;
    //Maintain the scope in which the variable is present
    std::uint16_t scope_index;

    ASTVariableAccess(const std::string& identifier, EvalType type, std::uint16_t scope_index)
        : identifier(identifier), var_type(type), scope_index(scope_index)
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    bool isFloat() const override {
        return var_type == EVAL_FLOAT;
    }

    EvalType evaluateExprType() const override {
        return var_type;
    }

    //Tag
    CTType getType() const override {
        return CTType::VarAccess;
    }
};

struct ASTCastNode : public ASTNode
{
    EvalType eval_type;
    ASTPtr   eval_expr;

    ASTCastNode(EvalType eval_type, ASTPtr&& expr)
        : eval_type(eval_type), eval_expr(std::move(expr))
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    bool isFloat() const override {
        return eval_type == EVAL_FLOAT;
    }

    EvalType evaluateExprType() const override {
        return eval_type;
    }

    //Tag
    CTType getType() const override {
        return CTType::Cast;
    }
};

//Can be used for multiple stuff, holding statements is used in alot of stuff like lists etc etc
struct ASTBlock : public ASTNode
{
    std::vector<ASTPtr> statements;

    ASTBlock(std::vector<ASTPtr>&& statements)
        : statements(std::move(statements))
    {}

    const std::vector<ASTPtr>& getStatements() const {
        return statements;
    }

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }
};

struct ASTTernaryOp : public ASTNode
{
    ASTPtr condition;
    ASTPtr true_expr;
    ASTPtr false_expr;

    ASTTernaryOp(ASTPtr&& condition, ASTPtr&& true_expr, ASTPtr&& false_expr)
        : condition(std::move(condition)), true_expr(std::move(true_expr)), false_expr(std::move(false_expr))
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    bool isFloat() const override {
        return true_expr->isFloat() || false_expr->isFloat();
    }

    bool isPowerOp() const override {
        return true_expr->isPowerOp() || false_expr->isPowerOp();
    }

    EvalType evaluateExprType() const override {
        //If this isnt float, then just return whatver the other expression is
        if (true_expr->evaluateExprType() == EVAL_FLOAT) {
            return EVAL_FLOAT;
        }
        return false_expr->evaluateExprType();
    }
};

//real
struct ASTIfNode : ASTNode
{
    //If
    ASTPtr if_condition;
    ASTPtr if_body;
    //Multiple Elif's or no Elif: pair -> condition, body
    std::vector<std::pair<ASTPtr, ASTPtr>> elif_clauses;
    //Else
    ASTPtr else_body;

    ASTIfNode(ASTPtr&& ifc, ASTPtr&& ifb, std::vector<std::pair<ASTPtr, ASTPtr>>&& elfc, ASTPtr&& eb = nullptr)
        : if_condition(std::move(ifc)), if_body(std::move(ifb)), elif_clauses(std::move(elfc)), else_body(std::move(eb))
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }
};

//--------------ITERATORS--------------
struct ASTBaseIterator : public ASTNode
{
    std::string iter_identifier;

    virtual ~ASTBaseIterator() = default;
};

//Range can be used for stuff like lists, for loop etc
struct ASTRangeIterator : public ASTBaseIterator
{
    //The way i want it is, start..stop..step, also it can be a binary expression so yeah
    ASTPtr start, stop, step;
    //0 is condition, 1 is construction (should range be used as condition or to contruct something)
    bool condition_or_construction = 0;

    ASTRangeIterator(ASTPtr&& start, ASTPtr&& stop, ASTPtr&& step, const std::string& iter_id, bool coc) //coc lmfao
        : start(std::move(start)), stop(std::move(stop)), step(std::move(step)), condition_or_construction(coc)
    {
        iter_identifier = iter_id;
    }

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    EvalType evaluateExprType() const override {
        return EVAL_RANGE_ITER;
    }

    EvalType evaluateIterType() const override {
        //Since step can be nullptr if it's being evaluated during runtime (Only in integer variation)
        if(step == nullptr)
            return EVAL_INT;
        
        //Whatever the step value type is will be the evaluated iter type as well
        return step->evaluateExprType();
    }
};

//--------------BREAK / CONTINUE--------------
struct ASTContinue : public ASTNode
{
    //For loop uses ITER_NEXT to go to next iteration, While loop doesn't
    std::uint8_t  continue_params;
    std::uint16_t scopes_to_destroy;

    ASTContinue(std::uint8_t continue_params, std::uint16_t scopes_to_destroy)
        : continue_params(continue_params), scopes_to_destroy(scopes_to_destroy)
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }
};

struct ASTBreak : public ASTNode
{
    //Only thing we check for in parameters is the IS_USING_SYMTBL
    std::uint8_t  break_params;
    std::uint16_t scopes_to_destroy;

    ASTBreak(std::uint8_t break_params, std::uint16_t scopes_to_destroy)
        : break_params(break_params), scopes_to_destroy(scopes_to_destroy)
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }
};

//--------------LOOPS--------------
struct ASTForNode : public ASTNode
{
    std::string id;
    ASTPtr      range;
    ASTPtr      for_body;

    ASTForNode(const std::string& id, ASTPtr&& range, ASTPtr&& for_body)
        : id(id), range(std::move(range)), for_body(std::move(for_body))
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }
};

struct ASTWhileNode : public ASTNode
{
    ASTPtr while_condition, while_body;

    ASTWhileNode(ASTPtr&& while_condition, ASTPtr&& while_body)
        : while_condition(std::move(while_condition)), while_body(std::move(while_body))
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }
};

#endif