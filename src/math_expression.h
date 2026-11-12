#pragma once

#include <memory>
#include <string>

namespace math_expression {

struct Node {
  char op;
  double value = 0.0;
  double (*func)(double) = nullptr;
  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;
};

using NodePtr = std::unique_ptr<Node>;

NodePtr parse(const char* text, std::string& error);
double evaluate(const Node* node, double x, double y, double a, double b);

} // namespace math_expression