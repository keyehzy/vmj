// Example of code:
//
// fn void foo() {
//   for(int i = 0; i < 10000000; i++) {
//   }
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
  // begin for loop
  auto i = std::make_unique<VariableDeclaration>("i", ValueType::Int, std::make_unique<Literal>(0));
  auto less_than =
      std::make_unique<LessThan>(std::make_unique<Variable>("i"), std::make_unique<Literal>(10000000));
  auto increment     = std::make_unique<Increment>(std::make_unique<Variable>("i"));
  auto for_loop_body = std::make_unique<Block>();
  auto for_loop = std::make_unique<For>(std::move(i), std::move(less_than), std::move(increment),
                                        std::move(for_loop_body));
  // end for loop
  function_decl->body->children.push_back(std::move(for_loop));
  // end function declaration

  function_decl->dump(std::cout);

  std::cout << AstInterpreter().interpret(*function_decl) << std::endl;
}