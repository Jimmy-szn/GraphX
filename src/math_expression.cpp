#include "math_expression.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>
namespace math_expression {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kE = 2.71828182845904523536;

// ---------------------------------------------------------------------------
// Equation parser
// Grammar (recursive descent):
//   expr    -> term (('+' | '-') term)*
//   term    -> unary (('*' | '/') unary)*
//   unary   -> ('-' | '+') unary | power
//   power   -> primary ('^' unary)?          so -x^2 = -(x^2) and 2^-x works
//   primary -> number | x | y | a | b | pi | e | func '(' expr ')' | '(' expr
//   ')'
// ---------------------------------------------------------------------------
NodePtr makeNode(char op, NodePtr left = nullptr, NodePtr right = nullptr) {
  auto node = std::make_unique<Node>();
  node->op = op;
  node->left = std::move(left);
  node->right = std::move(right);
  return node;
}

NodePtr makeNumber(double value) {
  NodePtr node = makeNode('n');
  node->value = value;
  return node;
}

struct Function {
  const char *name;
  double (*func)(double);
};

const Function kFunctions[] = {
    {"sin", [](double v) { return std::sin(v); }},
    {"cos", [](double v) { return std::cos(v); }},
    {"tan", [](double v) { return std::tan(v); }},
    {"sqrt", [](double v) { return std::sqrt(v); }},
    {"exp", [](double v) { return std::exp(v); }},
    {"log", [](double v) { return std::log(v); }},
    {"abs", [](double v) { return std::fabs(v); }},
};

class Parser {
public:
  explicit Parser(const char *text) : m_text(text) {}

  // Returns nullptr and fills `error` if the equation is invalid.
  NodePtr parse(std::string &error) {
    NodePtr root = parseExpr();
    skipSpaces();
    if (m_error.empty() && peek() != '\0')
      fail(std::string("Unexpected '") + peek() + "'");
    error = m_error;
    return m_error.empty() ? std::move(root) : nullptr;
  }

private:
  const char *m_text;
  std::size_t m_pos = 0;
  std::string m_error;

  char peek() const { return m_text[m_pos]; }
  void skipSpaces() {
    while (std::isspace(static_cast<unsigned char>(peek())))
      ++m_pos;
  }
  bool accept(char c) {
    skipSpaces();
    if (peek() != c)
      return false;
    ++m_pos;
    return true;
  }
  // Keeps the first error only; callers carry on with a dummy node.
  NodePtr fail(const std::string &message) {
    if (m_error.empty())
      m_error = message;
    return makeNumber(0.0);
  }

  NodePtr parseExpr() {
    NodePtr left = parseTerm();
    while (true) {
      if (accept('+'))
        left = makeNode('+', std::move(left), parseTerm());
      else if (accept('-'))
        left = makeNode('-', std::move(left), parseTerm());
      else
        return left;
    }
  }

  NodePtr parseTerm() {
    NodePtr left = parseUnary();
    while (true) {
      if (accept('*'))
        left = makeNode('*', std::move(left), parseUnary());
      else if (accept('/'))
        left = makeNode('/', std::move(left), parseUnary());
      else
        return left;
    }
  }

  NodePtr parseUnary() {
    if (accept('-'))
      return makeNode('u', parseUnary());
    if (accept('+'))
      return parseUnary();
    return parsePower();
  }

  NodePtr parsePower() {
    NodePtr base = parsePrimary();
    if (accept('^'))
      return makeNode('^', std::move(base), parseUnary());
    return base;
  }

  NodePtr parsePrimary() {
    skipSpaces();
    const char c = peek();

    if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
      char *end = nullptr;
      const double value = std::strtod(m_text + m_pos, &end);
      if (end == m_text + m_pos)
        return fail("Invalid number");
      m_pos = end - m_text;
      return makeNumber(value);
    }

    if (std::isalpha(static_cast<unsigned char>(c))) {
      std::string name;
      while (std::isalpha(static_cast<unsigned char>(peek())))
        name += m_text[m_pos++];

      if (name == "x" || name == "y" || name == "a" || name == "b")
        return makeNode(name[0]);
      if (name == "pi")
        return makeNumber(kPi);
      if (name == "e")
        return makeNumber(kE);
      for (const Function &f : kFunctions) {
        if (name != f.name)
          continue;
        if (!accept('('))
          return fail("Expected '(' after " + name);
        NodePtr n = makeNode('f', parseExpr());
        n->func = f.func;
        if (!accept(')'))
          return fail("Missing ')' after " + name + "(...");
        return n;
      }
      return fail("Unknown name '" + name + "'");
    }

    if (accept('(')) {
      NodePtr inner = parseExpr();
      if (!accept(')'))
        return fail("Missing ')'");
      return inner;
    }

    if (c == '\0')
      return fail("Equation ended unexpectedly");
    return fail(std::string("Unexpected '") + c + "'");
  }
};
} // namespace
double evaluate(const Node *n, double x, double y, double a, double b) {
  switch (n->op) {
  case 'n':
    return n->value;
  case 'x':
    return x;
  case 'y':
    return y;
  case 'a':
    return a;
  case 'b':
    return b;
  case 'u':
    return -evaluate(n->left.get(), x, y, a, b);
  case 'f':
    return n->func(evaluate(n->left.get(), x, y, a, b));
  }
  const double l = evaluate(n->left.get(), x, y, a, b);
  const double r = evaluate(n->right.get(), x, y, a, b);
  switch (n->op) {
  case '+':
    return l + r;
  case '-':
    return l - r;
  case '*':
    return l * r;
  case '/':
    return l / r;
  case '^':
    return std::pow(l, r);
  }
  return 0.0;
}
NodePtr parse(const char *text, std::string &error) {
  return Parser(text).parse(error);
}

} // namespace math_expression
