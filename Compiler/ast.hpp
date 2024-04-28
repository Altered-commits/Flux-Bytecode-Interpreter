#ifndef UNNAMED_AST_HPP
#define UNNAMED_AST_HPP

#include <memory>
#include <string>
#include "token.hpp"

struct ASTNode;
using  ASTPtr = std::unique_ptr<ASTNode>;

//For the visitor pattern, we need to forward declare all the nodes, here we go
struct ASTValue;
struct ASTBinaryOp;
struct ASTUnaryOp;
struct ASTVariableAssign;
struct ASTVariableAccess;
struct ASTCastDummy;

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

    //Expression evaluation for final type (variables)
    virtual TokenType evaluateExprType() const = 0;
};

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
        if(op_type == TOKEN_POW)
            return TOKEN_FLOAT;
        
        if(left->evaluateExprType() == TOKEN_FLOAT || right->evaluateExprType() == TOKEN_FLOAT)
            return TOKEN_FLOAT;

        return TOKEN_INT;
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
        return expr->evaluateExprType();
    }
};

struct ASTVariableAssign : public ASTNode
{
    std::string identifier;
    ASTPtr      expr;
    TokenType   var_type;
    bool        should_return_value;

    ASTVariableAssign(const std::string& identifier, TokenType type, ASTPtr&& expr, bool srv = false)
        : identifier(identifier), var_type(type), expr(std::move(expr)), should_return_value(srv)
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

#endif