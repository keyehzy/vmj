#include "ast.h"

std::string to_string(ValueType type) {
  switch (type) {
    case ValueType::Void:
      return "void";
    case ValueType::Int:
      return "int";
    case ValueType::Float:
      return "float";
    case ValueType::Bool:
      return "bool";
  }
  return "unknown";
}