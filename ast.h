#pragma once

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class AstType {
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
  Add,
};

struct Ast {
  struct FunctionDeclaration;
  struct Block;
  struct While;
  struct VariableDeclaration;
  struct LessThan;
  struct Increment;
  struct Literal;
  struct Variable;
  struct Assignment;
  struct Return;
  struct IfElse;
  struct Add;

  AstType type;

  Ast() = default;

  virtual ~Ast() = default;

  virtual void dump(std::ostream &os) const = 0;

 protected:
  explicit Ast(AstType type) : type(type) {}
};

template <typename Derived_Reference, typename Base>
Derived_Reference derived_cast(Base &base) {
  using Derived = std::remove_reference_t<Derived_Reference>;
  static_assert(std::is_base_of_v<Base, Derived>);
  static_assert(!std::is_base_of_v<Derived, Base>);
  return static_cast<Derived_Reference>(base);
}

template <typename Derived>
Derived ast_cast(Ast &ast) {
  return derived_cast<Derived>(ast);
}

template <typename Derived>
Derived ast_cast(const Ast &ast) {
  return derived_cast<Derived>(ast);
}

struct Ast::Block final : public Ast {
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

struct Ast::FunctionDeclaration final : public Ast {
  std::string name;
  ValueType return_type;
  std::unique_ptr<Block> body;

  FunctionDeclaration(std::string name, ValueType return_type, std::unique_ptr<Block> body)
      : Ast(AstType::FunctionDeclaration),
        name(name),
        return_type(return_type),
        body(std::move(body)) {}

  void dump(std::ostream &os) const override {
    os << "FunctionDeclaration(" << name << ", " << to_string(return_type) << ", ";
    body->dump(os);
    os << ")";
  }
};

struct Ast::VariableDeclaration final : public Ast {
  std::string name;
  ValueType type;
  std::unique_ptr<Ast> initializer;

  VariableDeclaration(std::string name, ValueType type, std::unique_ptr<Ast> initializer)
      : Ast(AstType::VariableDeclaration),
        name(name),
        type(type),
        initializer(std::move(initializer)) {}

  void dump(std::ostream &os) const override {
    os << "VariableDeclaration(" << name << ", " << to_string(type) << ", ";
    initializer->dump(os);
    os << ")";
  }
};

struct Ast::LessThan final : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  LessThan(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(AstType::LessThan), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override {
    os << "LessThan(";
    left->dump(os);
    os << ", ";
    right->dump(os);
    os << ")";
  }
};

struct Ast::Literal final : public Ast {
  int value;

  Literal(int value) : Ast(AstType::Literal), value(value) {}

  void dump(std::ostream &os) const override { os << "Literal(" << value << ")"; }
};

struct Ast::Variable final : public Ast {
  std::string name;

  Variable(std::string name) : Ast(AstType::Variable), name(name) {}

  void dump(std::ostream &os) const override { os << "Variable(" << name << ")"; }
};

struct Ast::Increment final : public Ast {
  std::unique_ptr<Variable> variable;

  Increment(std::unique_ptr<Variable> variable)
      : Ast(AstType::Increment), variable(std::move(variable)) {}

  void dump(std::ostream &os) const override { os << "Increment(" << variable->name << ")"; }
};

struct Ast::While final : public Ast {
  std::unique_ptr<LessThan> condition;
  std::unique_ptr<Block> body;

  While(std::unique_ptr<LessThan> condition, std::unique_ptr<Block> body)
      : Ast(AstType::While), condition(std::move(condition)), body(std::move(body)) {}

  void dump(std::ostream &os) const override {
    os << "While(";
    condition->dump(os);
    os << ", ";
    body->dump(os);
    os << ")";
  }
};

struct Ast::Assignment final : public Ast {
  std::string name;
  std::unique_ptr<Ast> value;

  Assignment(std::string name, std::unique_ptr<Ast> value)
      : Ast(AstType::Assignment), name(name), value(std::move(value)) {}

  void dump(std::ostream &os) const override {
    os << "Assignment(" << name << ", ";
    value->dump(os);
    os << ")";
  }
};

struct Ast::Return final : public Ast {
  std::unique_ptr<Ast> value;

  Return(std::unique_ptr<Ast> value) : Ast(AstType::Return), value(std::move(value)) {}

  void dump(std::ostream &os) const override {
    os << "Return(";
    value->dump(os);
    os << ")";
  }
};

struct Ast::IfElse final : public Ast {
  std::unique_ptr<LessThan> condition;
  std::unique_ptr<Block> body;
  std::unique_ptr<Block> else_body;

  IfElse(std::unique_ptr<LessThan> condition, std::unique_ptr<Block> body,
         std::unique_ptr<Block> else_body)
      : Ast(AstType::IfElse),
        condition(std::move(condition)),
        body(std::move(body)),
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

struct Ast::Add final : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  Add(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(AstType::Add), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override {
    os << "Add(";
    left->dump(os);
    os << ", ";
    right->dump(os);
    os << ")";
  }
};

struct AstInterpreter {
  int interpret_variable(const Ast::Variable &variable) { return variables[variable.name]; }

  int interpret_literal(const Ast::Literal &literal) { return literal.value; }

  int interpret_less_than(const Ast::LessThan &less_than) {
    return interpret(*less_than.left) < interpret(*less_than.right);
  }

  int interpret_variable_declaration(const Ast::VariableDeclaration &variable_declaration) {
    // make sure the variable is not already declared
    assert(variables.find(variable_declaration.name) == variables.end());
    // FIXME: check type of initializer
    return variables[variable_declaration.name] = interpret(*variable_declaration.initializer);
  }

  int interpret_increment(const Ast::Increment &increment) {
    return variables[increment.variable->name]++;
  }

  int interpret_block(const Ast::Block &block) {
    int result = 0;
    for (const auto &child : block.children) {
      result = interpret(*child);
    }
    return result;
  }

  int interpret_while(const Ast::While &while_loop) {
    int result = 0;
    while (interpret(*while_loop.condition)) {
      result = interpret_block(*while_loop.body);
    }
    return result;
  }

  int interpret_function_declaration(const Ast::FunctionDeclaration &function_declaration) {
    return interpret_block(*function_declaration.body);
  }

  int interpret_assignment(const Ast::Assignment &assignment) {
    // make sure the variable is already declared
    assert(variables.find(assignment.name) != variables.end());
    // FIXME: check type of value
    return variables[assignment.name] = interpret(*assignment.value);
  }

  int interpret_return(const Ast::Return &return_statement) {
    // FIXME: check type of return value
    return interpret(*return_statement.value);
  }

  int interpret_if_else(const Ast::IfElse &if_else) {
    if (interpret(*if_else.condition)) {
      return interpret_block(*if_else.body);
    } else {
      return interpret_block(*if_else.else_body);
    }
  }

  int interpret_add(const Ast::Add &add) { return interpret(*add.left) + interpret(*add.right); }

  int interpret(const Ast &ast) {
    switch (ast.type) {
      case AstType::Variable:
        return interpret_variable(ast_cast<Ast::Variable const &>(ast));
      case AstType::Literal:
        return interpret_literal(ast_cast<Ast::Literal const &>(ast));
      case AstType::LessThan:
        return interpret_less_than(ast_cast<Ast::LessThan const &>(ast));
      case AstType::VariableDeclaration:
        return interpret_variable_declaration(ast_cast<Ast::VariableDeclaration const &>(ast));
      case AstType::Increment:
        return interpret_increment(ast_cast<Ast::Increment const &>(ast));
      case AstType::While:
        return interpret_while(ast_cast<Ast::While const &>(ast));
      case AstType::Block:
        return interpret_block(ast_cast<Ast::Block const &>(ast));
      case AstType::FunctionDeclaration:
        return interpret_function_declaration(ast_cast<Ast::FunctionDeclaration const &>(ast));
      case AstType::Assignment:
        return interpret_assignment(ast_cast<Ast::Assignment const &>(ast));
      case AstType::Return:
        return interpret_return(ast_cast<Ast::Return const &>(ast));
      case AstType::IfElse:
        return interpret_if_else(ast_cast<Ast::IfElse const &>(ast));
      case AstType::Add:
        return interpret_add(ast_cast<Ast::Add const &>(ast));
    }
    assert(false);
  }

  std::unordered_map<std::string, int> variables;
};