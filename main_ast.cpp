// Example of code:
//
// fn void foo() {
//   int n = 10;
//   int i = 0;
//   int t1 = 0;
//   int t2 = 1;
//   int t3 = 0;
//   for (int i = 0; i < n; i++) {
//     t3 = t1 + t2;
//     t1 = t2;
//     t2 = t3;
//   }
//   return t1;
// }

#include "ast.h"

int main() {
  // begin function declaration
  auto function_decl = std::make_unique<Ast::FunctionDeclaration>("foo", ValueType::Void,
                                                                  std::make_unique<Ast::Block>());
  auto &decl_body    = function_decl->body;

  // declare n, i, t1, t2, t3
  decl_body->append<Ast::VariableDeclaration>("n", ValueType::Int,
                                              std::make_unique<Ast::Literal>(20));
  decl_body->append<Ast::VariableDeclaration>("i", ValueType::Int,
                                              std::make_unique<Ast::Literal>(0));
  decl_body->append<Ast::VariableDeclaration>("t1", ValueType::Int,
                                              std::make_unique<Ast::Literal>(0));
  decl_body->append<Ast::VariableDeclaration>("t2", ValueType::Int,
                                              std::make_unique<Ast::Literal>(1));
  decl_body->append<Ast::VariableDeclaration>("t3", ValueType::Int,
                                              std::make_unique<Ast::Literal>(0));

  // for (int i = 0; i < n; i++) {
  //   t3 = t1 + t2;
  //   t1 = t2;
  //   t2 = t3;
  // }
  auto less_than = std::make_unique<Ast::LessThan>(std::make_unique<Ast::Variable>("i"),
                                                   std::make_unique<Ast::Variable>("n"));
  auto block     = std::make_unique<Ast::Block>();
  block->append<Ast::Assignment>("t3",
                                 std::make_unique<Ast::Add>(std::make_unique<Ast::Variable>("t1"),
                                                            std::make_unique<Ast::Variable>("t2")));
  block->append<Ast::Assignment>("t1", std::make_unique<Ast::Variable>("t2"));
  block->append<Ast::Assignment>("t2", std::make_unique<Ast::Variable>("t3"));
  block->append<Ast::Increment>(std::make_unique<Ast::Variable>("i"));
  auto while_loop = std::make_unique<Ast::While>(std::move(less_than), std::move(block));
  decl_body->children.push_back(std::move(while_loop));
  // end for loop

  decl_body->append<Ast::Return>(std::make_unique<Ast::Variable>("t1"));
  // end function declaration

  function_decl->dump(std::cout);
  std::cout << AstInterpreter().interpret(*function_decl) << std::endl;
}