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
    For,
    VariableDeclaration,
    LessThan,
    Increment,
    Literal,
    Variable,
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

struct For : public Ast {
  std::unique_ptr<VariableDeclaration> initializer;
  std::unique_ptr<LessThan> condition;
  std::unique_ptr<Increment> increment;
  std::unique_ptr<Block> body;

  For(std::unique_ptr<VariableDeclaration> initializer, std::unique_ptr<LessThan> condition,
      std::unique_ptr<Increment> increment, std::unique_ptr<Block> body)
      : Ast(Type::For),
        initializer(std::move(initializer)),
        condition(std::move(condition)),
        increment(std::move(increment)),
        body(std::move(body)) {}

  void dump(std::ostream &os) const override {
    os << "For(";
    initializer->dump(os);
    os << ", ";
    condition->dump(os);
    os << ", ";
    increment->dump(os);
    os << ", ";
    body->dump(os);
  }
};

struct AstInterpreter {
  int interpret_variable(const Variable &variable) { return variables[variable.name]; }

  int interpret_literal(const Literal &literal) { return literal.value; }

  int interpret_less_than(const LessThan &less_than) {
    return interpret(*less_than.left) < interpret(*less_than.right);
  }

  int interpret_variable_declaration(const VariableDeclaration &variable_declaration) {
    return variables[variable_declaration.name] = interpret(*variable_declaration.initializer);
  }

  int interpret_increment(const Increment &increment) {
    return variables[increment.variable->name]++;
  }

  int interpret_block(const Block &block) {
    for (const auto &child : block.children) {
      interpret(*child);
    }
    return 0;
  }

  int interpret_for(const For &for_loop) {
    for (int i = interpret(*for_loop.initializer); interpret(*for_loop.condition); i++) {
      interpret_block(*for_loop.body);
      interpret(*for_loop.increment);
    }
    return 0;
  }

  int interpret_function_declaration(const FunctionDeclaration &function_declaration) {
    return interpret_block(*function_declaration.body);
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
      case Ast::Type::For:
        return interpret_for(static_cast<const For &>(ast));
      case Ast::Type::Block:
        return interpret_block(static_cast<const Block &>(ast));
      case Ast::Type::FunctionDeclaration:
        return interpret_function_declaration(static_cast<const FunctionDeclaration &>(ast));
    }
    assert(false);
  }

  std::unordered_map<std::string, int> variables;
};