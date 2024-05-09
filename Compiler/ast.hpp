#ifndef UNNAMED_AST_HPP
#define UNNAMED_AST_HPP

#include <memory>
#include <string>
#include "token.hpp"
#include "error_printer.hpp"

//Forward declare cuz the below std::unique_ptr is going to cry if not done
struct ASTNode;

using ASTPtr    = std::unique_ptr<ASTNode>;
using ASTRawPtr = ASTNode*;

//BTW, this may look useless right now but trust me its important for same precedence operators
constexpr size_t comparisionTypeMask = (1ULL << static_cast<std::size_t>(TOKEN_EEQ))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_NEQ))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_GT))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_LT))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_GTEQ))
                                    | (1ULL << static_cast<std::size_t>(TOKEN_LTEQ));

//Used for comparing token type in a type mask
#define SAME_PRECEDENCE_MASK_CHECK(token, mask) (((1ULL << static_cast<std::size_t>(token)) & mask))

//Some enums for Compile time evaluation
enum class CTType {
    NA,
    Value,
    Binary,
    Unary,
    VarAccess
};

//For the visitor pattern, we need to forward declare all the nodes, here we go
struct ASTValue;
struct ASTBinaryOp;
struct ASTUnaryOp;
struct ASTVariableAssign;
struct ASTVariableAccess;
struct ASTCastDummy;
struct ASTBlock;
struct ASTTernaryOp;
struct ASTIfNode;
struct ASTRangeIterator;
struct ASTForNode;

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
        virtual void visit(ASTCastDummy& node, bool)      = 0;
        virtual void visit(ASTBlock& node, bool)          = 0;
        virtual void visit(ASTTernaryOp& node, bool)      = 0;
        virtual void visit(ASTIfNode& node, bool)         = 0;
        virtual void visit(ASTRangeIterator& node, bool)  = 0;
        virtual void visit(ASTForNode& node, bool)        = 0;
};

//AST nodes
struct ASTNode
{
    virtual ~ASTNode() = default;

    //Visitor
    virtual void accept(ASTVisitorInterface& visitor, bool is_sub_expr) = 0;

    //AST Checks
    virtual bool isFloat() const = 0;
    virtual bool isPowerOp() const = 0;

    //Think of it as a tag for each type
    virtual CTType getType() const { return CTType::NA; }

    //Expression evaluation for final type (variables and range)
    virtual TokenType evaluateExprType() const = 0;
    virtual TokenType evaluateIterType() const { return TOKEN_UNKNOWN; }
};

//-----------------------------AST Types-----------------------------
struct ASTValue : public ASTNode
{
    TokenType   type;
    std::string value;

    ASTValue(TokenType type, const std::string& value)
        : type(type), value(value)
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    bool isFloat() const override {
        return type == TOKEN_FLOAT;
    }
    bool isPowerOp() const override {
        return false;
    }

    TokenType evaluateExprType() const override {
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

    TokenType evaluateExprType() const override {
        switch (op_type)
        {
            case TOKEN_POW:
                return TOKEN_FLOAT;
            
            case TOKEN_OR:
            case TOKEN_AND:
                return TOKEN_INT;
        }
        
        if(SAME_PRECEDENCE_MASK_CHECK(op_type, comparisionTypeMask))
            return TOKEN_INT;

        if(left->evaluateExprType() == TOKEN_FLOAT || right->evaluateExprType() == TOKEN_FLOAT)
            return TOKEN_FLOAT;

        return TOKEN_INT;
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

    TokenType evaluateExprType() const override {
        if(op_type == TOKEN_NOT)
            return TOKEN_INT;
        return expr->evaluateExprType();
    }

    //Tag
    CTType getType() const override {
        return CTType::Unary;
    }
};

struct ASTVariableAssign : public ASTNode
{
    std::string identifier;
    ASTPtr      expr;
    TokenType   var_type;
    bool        is_reassignment;

    ASTVariableAssign(const std::string& identifier, TokenType type, ASTPtr&& expr, bool is_reassignment)
        : identifier(identifier), var_type(type), expr(std::move(expr)), is_reassignment(is_reassignment)
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

    TokenType evaluateExprType() const override {
        return var_type == TOKEN_KEYWORD_FLOAT ? TOKEN_FLOAT : TOKEN_INT;
    }
};

struct ASTVariableAccess : public ASTNode
{
    std::string identifier;
    TokenType   var_type;

    ASTVariableAccess(const std::string& identifier, TokenType type)
        : identifier(identifier), var_type(type)
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    bool isFloat() const override {
        return var_type == TOKEN_KEYWORD_FLOAT;
    }
    bool isPowerOp() const override {
        return false;
    }

    TokenType evaluateExprType() const override {
        return var_type == TOKEN_KEYWORD_FLOAT ? TOKEN_FLOAT : TOKEN_INT;
    }

    //Tag
    CTType getType() const override {
        return CTType::VarAccess;
    }
};

struct ASTCastDummy : public ASTNode
{
    TokenType eval_type;
    ASTPtr    eval_expr;

    ASTCastDummy(TokenType eval_type, ASTPtr&& expr)
        : eval_type(eval_type), eval_expr(std::move(expr))
    {}

    void accept(ASTVisitorInterface& visitor, bool is_sub_expr) override {
        visitor.visit(*this, is_sub_expr);
    }

    bool isFloat() const override {
        return eval_type == TOKEN_KEYWORD_FLOAT;
    }
    bool isPowerOp() const override {
        return false;
    }

    TokenType evaluateExprType() const override {
        return eval_type == TOKEN_KEYWORD_FLOAT ? TOKEN_FLOAT : TOKEN_INT;
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

    bool isFloat() const override {
        return false;
    }
    bool isPowerOp() const override {
        return false;
    }

    TokenType evaluateExprType() const override {
        return TOKEN_UNKNOWN;
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

    TokenType evaluateExprType() const override {
        //If this isnt float, then just return whatver the other expression is
        if (true_expr->evaluateExprType() == TOKEN_FLOAT) {
            return TOKEN_FLOAT;
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

    bool isFloat() const override {
        return false;
    }
    bool isPowerOp() const override {
        return false;
    }

    TokenType evaluateExprType() const override {
        return TOKEN_UNKNOWN;
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

    bool isFloat() const override {
        return false;
    }
    bool isPowerOp() const override {
        return false;
    }

    TokenType evaluateExprType() const override {
        return TOKEN_RANGE_ITER;
    }

    TokenType evaluateIterType() const override {
        TokenType tsr = start->evaluateExprType();
        TokenType tso = stop->evaluateExprType();

        return (tsr == TOKEN_FLOAT || tso == TOKEN_FLOAT) ? TOKEN_FLOAT : TOKEN_INT;
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

    bool isFloat() const override {
        return false;
    }
    bool isPowerOp() const override {
        return false;
    }

    TokenType evaluateExprType() const override {
        return TOKEN_UNKNOWN;
    }
};

#endif