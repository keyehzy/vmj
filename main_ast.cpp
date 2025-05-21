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
  auto function_decl =
      std::make_unique<FunctionDeclaration>("foo", ValueType::Void, std::make_unique<Block>());
  auto &decl_body = function_decl->body;

  // declare n, i, t1, t2, t3
  decl_body->append<VariableDeclaration>("n", ValueType::Int, std::make_unique<Literal>(20));
  decl_body->append<VariableDeclaration>("i", ValueType::Int, std::make_unique<Literal>(0));
  decl_body->append<VariableDeclaration>("t1", ValueType::Int, std::make_unique<Literal>(0));
  decl_body->append<VariableDeclaration>("t2", ValueType::Int, std::make_unique<Literal>(1));
  decl_body->append<VariableDeclaration>("t3", ValueType::Int, std::make_unique<Literal>(0));

  // for (int i = 0; i < n; i++) {
  //   t3 = t1 + t2;
  //   t1 = t2;
  //   t2 = t3;
  // }
  auto less_than =
      std::make_unique<LessThan>(std::make_unique<Variable>("i"), std::make_unique<Variable>("n"));
  auto block = std::make_unique<Block>();
  block->append<Assignment>("t3", std::make_unique<Add>(std::make_unique<Variable>("t1"),
                                                        std::make_unique<Variable>("t2")));
  block->append<Assignment>("t1", std::make_unique<Variable>("t2"));
  block->append<Assignment>("t2", std::make_unique<Variable>("t3"));
  block->append<Increment>(std::make_unique<Variable>("i"));
  auto while_loop = std::make_unique<While>(std::move(less_than), std::move(block));
  decl_body->children.push_back(std::move(while_loop));
  // end for loop

  decl_body->append<Return>(std::make_unique<Variable>("t1"));
  // end function declaration

  function_decl->dump(std::cout);
  std::cout << AstInterpreter().interpret(*function_decl) << std::endl;
}