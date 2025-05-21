// Example of code:
//
// fn void foo() {
//   int j = 0;
//   for(int i = 0; i < 10000000; i++) {
//     j = i;
//   }
//   return j;
// }

// AST:
// Program
//   - FunctionDeclaration
//     - ReturnType: void
//     - Name: foo
//     - Body:
//       - For
//         - Initializer: VariableDeclaration
//           - Type: int
//           - Name: i
//           - Initializer: Literal(0)
//         - Condition: LessThan
//           - Left: Variable(i)
//           - Right: Literal(10)
//         - Body:
//           - Block

#include "ast.h"

int main() {
  // begin function declaration
  auto function_decl =
      std::make_unique<FunctionDeclaration>("foo", ValueType::Void, std::make_unique<Block>());

  // declare j
  function_decl->body->append<VariableDeclaration>("j", ValueType::Int,
                                                   std::make_unique<Literal>(0));

  // begin for loop (as a while loop)
  function_decl->body->append<VariableDeclaration>("i", ValueType::Int,
                                                   std::make_unique<Literal>(0));
  auto less_than =
      std::make_unique<LessThan>(std::make_unique<Variable>("i"), std::make_unique<Literal>(1000));
  auto while_loop_body = std::make_unique<Block>();
  while_loop_body->append<Assignment>("j", std::make_unique<Variable>("i"));
  while_loop_body->append<Increment>(std::make_unique<Variable>("i"));
  auto while_loop = std::make_unique<While>(std::move(less_than), std::move(while_loop_body));
  function_decl->body->children.push_back(std::move(while_loop));
  // end while loop

  function_decl->body->append<Return>(std::make_unique<Variable>("j"));
  // end function declaration

  function_decl->dump(std::cout);
  std::cout << AstInterpreter().interpret(*function_decl) << std::endl;
}