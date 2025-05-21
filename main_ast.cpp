// Example of code:
//
// fn void foo() {
//   int i = 42;
//   int j = 0;
//   if (i < 100) {
//     j = i;
//   }
//   return j;
// }

#include "ast.h"

int main() {
  // begin function declaration
  auto function_decl =
      std::make_unique<FunctionDeclaration>("foo", ValueType::Void, std::make_unique<Block>());

  // declare i
  function_decl->body->append<VariableDeclaration>("i", ValueType::Int,
                                                   std::make_unique<Literal>(42));

  // declare j
  function_decl->body->append<VariableDeclaration>("j", ValueType::Int,
                                                   std::make_unique<Literal>(0));

  // if (i < 100) {
  //   j = i;
  // }
  auto less_than =
      std::make_unique<LessThan>(std::make_unique<Variable>("i"), std::make_unique<Literal>(100));
  auto if_block = std::make_unique<Block>();
  if_block->append<Assignment>("j", std::make_unique<Variable>("i"));
  auto else_block = std::make_unique<Block>();
  auto if_else =
      std::make_unique<IfElse>(std::move(less_than), std::move(if_block), std::move(else_block));
  function_decl->body->children.push_back(std::move(if_else));
  // end if

  function_decl->body->append<Return>(std::make_unique<Variable>("j"));
  // end function declaration

  function_decl->dump(std::cout);
  std::cout << AstInterpreter().interpret(*function_decl) << std::endl;
}