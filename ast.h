#pragma once

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Ast {
  enum class Type {
    FunctionDeclaration,
    Block,
    While,
    VariableDeclaration,
    LessThan,
    Increment,
    Literal,
    Variable,
    Assignment,
    Return,
    IfElse,
  };

  Type type;

  Ast()          = default;
  virtual ~Ast() = default;

  virtual void dump(std::ostream &os) const = 0;

 protected:
  explicit Ast(Type type) : type(type) {}
};

struct Block : public Ast {
  std::vector<std::unique_ptr<Ast>> children;

  template <typename T, typename... Args>
  void append(Args &&...args) {
    children.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
  }

  void dump(std::ostream &os) const override {
    os << "Block(";
    for (const auto &child : children) {
      child->dump(os);
    }
    os << ")";
  }
};

enum class ValueType {
  Void,
  Int,
  Float,
  Bool,
};

std::string to_string(ValueType type);

struct FunctionDeclaration : public Ast {
  std::string name;
  ValueType return_type;
  std::unique_ptr<Block> body;

  FunctionDeclaration(std::string name, ValueType return_type, std::unique_ptr<Block> body)
      : Ast(Type::FunctionDeclaration),
        name(name),
        return_type(return_type),
        body(std::move(body)) {}

  void dump(std::ostream &os) const override {
    os << "FunctionDeclaration(" << name << ", " << to_string(return_type) << ", ";
    body->dump(os);
    os << ")";
  }
};

struct VariableDeclaration : public Ast {
  std::string name;
  ValueType type;
  std::unique_ptr<Ast> initializer;

  VariableDeclaration(std::string name, ValueType type, std::unique_ptr<Ast> initializer)
      : Ast(Type::VariableDeclaration),
        name(name),
        type(type),
        initializer(std::move(initializer)) {}

  void dump(std::ostream &os) const override {
    os << "VariableDeclaration(" << name << ", " << to_string(type) << ", ";
    initializer->dump(os);
    os << ")";
  }
};

struct LessThan : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  LessThan(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(Type::LessThan), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override {
    os << "LessThan(";
    left->dump(os);
    os << ", ";
    right->dump(os);
    os << ")";
  }
};

struct Literal : public Ast {
  int value;

  Literal(int value) : Ast(Type::Literal), value(value) {}

  void dump(std::ostream &os) const override { os << "Literal(" << value << ")"; }
};

struct Variable : public Ast {
  std::string name;

  Variable(std::string name) : Ast(Type::Variable), name(name) {}

  void dump(std::ostream &os) const override { os << "Variable(" << name << ")"; }
};

struct Increment : public Ast {
  std::unique_ptr<Variable> variable;

  Increment(std::unique_ptr<Variable> variable)
      : Ast(Type::Increment), variable(std::move(variable)) {}

  void dump(std::ostream &os) const override { os << "Increment(" << variable->name << ")"; }
};

struct While : public Ast {
  std::unique_ptr<LessThan> condition;
  std::unique_ptr<Block> body;

  While(std::unique_ptr<LessThan> condition, std::unique_ptr<Block> body)
      : Ast(Type::While), condition(std::move(condition)), body(std::move(body)) {}

  void dump(std::ostream &os) const override {
    os << "While(";
    condition->dump(os);
    os << ", ";
    body->dump(os);
    os << ")";
  }
};

struct Assignment : public Ast {
  std::string name;
  std::unique_ptr<Ast> value;

  Assignment(std::string name, std::unique_ptr<Ast> value)
      : Ast(Type::Assignment), name(name), value(std::move(value)) {}

  void dump(std::ostream &os) const override {
    os << "Assignment(" << name << ", ";
    value->dump(os);
    os << ")";
  }
};

struct Return : public Ast {
  std::unique_ptr<Ast> value;

  Return(std::unique_ptr<Ast> value) : Ast(Type::Return), value(std::move(value)) {}

  void dump(std::ostream &os) const override {
    os << "Return(";
    value->dump(os);
    os << ")";
  }
};

struct IfElse : public Ast {
  std::unique_ptr<LessThan> condition;
  std::unique_ptr<Block> body;
  std::unique_ptr<Block> else_body;

  IfElse(std::unique_ptr<LessThan> condition, std::unique_ptr<Block> body,
         std::unique_ptr<Block> else_body)
      : Ast(Type::IfElse), condition(std::move(condition)), body(std::move(body)),
        else_body(std::move(else_body)) {}

  void dump(std::ostream &os) const override {
    os << "IfElse(";
    condition->dump(os);
    os << ", ";
    body->dump(os);
    os << ", ";
    else_body->dump(os);
    os << ")";
  }
};

struct AstInterpreter {
  int interpret_variable(const Variable &variable) { return variables[variable.name]; }

  int interpret_literal(const Literal &literal) { return literal.value; }

  int interpret_less_than(const LessThan &less_than) {
    return interpret(*less_than.left) < interpret(*less_than.right);
  }

  int interpret_variable_declaration(const VariableDeclaration &variable_declaration) {
    // make sure the variable is not already declared
    assert(variables.find(variable_declaration.name) == variables.end());
    // FIXME: check type of initializer
    return variables[variable_declaration.name] = interpret(*variable_declaration.initializer);
  }

  int interpret_increment(const Increment &increment) {
    return variables[increment.variable->name]++;
  }

  int interpret_block(const Block &block) {
    int result = 0;
    for (const auto &child : block.children) {
      result = interpret(*child);
    }
    return result;
  }

  int interpret_while(const While &while_loop) {
    int result = 0;
    while (interpret(*while_loop.condition)) {
      result = interpret_block(*while_loop.body);
    }
    return result;
  }

  int interpret_function_declaration(const FunctionDeclaration &function_declaration) {
    return interpret_block(*function_declaration.body);
  }

  int interpret_assignment(const Assignment &assignment) {
    // make sure the variable is already declared
    assert(variables.find(assignment.name) != variables.end());
    // FIXME: check type of value
    return variables[assignment.name] = interpret(*assignment.value);
  }

  int interpret_return(const Return &return_statement) {
    // FIXME: check type of return value
    return interpret(*return_statement.value);
  }

  int interpret_if_else(const IfElse &if_else) {
    if (interpret(*if_else.condition)) {
      return interpret_block(*if_else.body);
    } else {
      return interpret_block(*if_else.else_body);
    }
  }

  int interpret(const Ast &ast) {
    switch (ast.type) {
      case Ast::Type::Variable:
        return interpret_variable(static_cast<const Variable &>(ast));
      case Ast::Type::Literal:
        return interpret_literal(static_cast<const Literal &>(ast));
      case Ast::Type::LessThan:
        return interpret_less_than(static_cast<const LessThan &>(ast));
      case Ast::Type::VariableDeclaration:
        return interpret_variable_declaration(static_cast<const VariableDeclaration &>(ast));
      case Ast::Type::Increment:
        return interpret_increment(static_cast<const Increment &>(ast));
      case Ast::Type::While:
        return interpret_while(static_cast<const While &>(ast));
      case Ast::Type::Block:
        return interpret_block(static_cast<const Block &>(ast));
      case Ast::Type::FunctionDeclaration:
        return interpret_function_declaration(static_cast<const FunctionDeclaration &>(ast));
      case Ast::Type::Assignment:
        return interpret_assignment(static_cast<const Assignment &>(ast));
      case Ast::Type::Return:
        return interpret_return(static_cast<const Return &>(ast));
      case Ast::Type::IfElse:
        return interpret_if_else(static_cast<const IfElse &>(ast));
    }
    assert(false);
  }

  std::unordered_map<std::string, int> variables;
};