#include <cassert>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

struct Location {
  std::string file_path;
  int row;
  int col;
};

enum class TokenType {
  IDENTITY,
  OPEN_PAREN,
  CLOSE_PAREN,
  OPEN_CURLY,
  CLOSE_CURLY,
  SEMICOLON,
  NUMBER,
  STRING,
  RETURN
};

static const char *token_type_names[9] = {
    "IDENTITY",  "OPEN_PAREN", "CLOSE_PAREN", "OPEN_CURLY", "CLOSE_CURLY",
    "SEMICOLON", "NUMBER",     "STRING",      "RETURN",
};

static const std::unordered_map<char, TokenType> token_type_map = {
    {'(', TokenType::OPEN_PAREN}, {')', TokenType::CLOSE_PAREN},
    {'{', TokenType::OPEN_CURLY}, {'}', TokenType::CLOSE_CURLY},
    {';', TokenType::SEMICOLON},
};

static bool is_in_token_map(char key) {
  return token_type_map.find(key) != token_type_map.end();
}

struct Token {
  TokenType type;
  std::optional<std::string> text;
  std::optional<int> number;
  Location location;
};

class Lexer {
  std::string source_path;
  std::ifstream source;
  int line_begin;
  int row;

  void chop_char() {
    if (source.good() && source.get() == '\n') {
      line_begin = static_cast<int>(source.tellg()) + 1;
      ++row;
    }
  }

  void trim_left() {
    while (source.good() && std::isspace(source.peek()))
      chop_char();
  }

  void drop_line() {
    while (source.good() && source.peek() != '\n')
      chop_char();
    if (source.good())
      chop_char();
  }

public:
  Lexer(const std::string &source_path)
      : source_path(source_path), line_begin(0), row(0) {
    source.open(source_path);
  }

  ~Lexer() { source.close(); }

  std::optional<Token> next_token() {
    trim_left();

    while (source.good() && source.peek() == '#') {
      drop_line();
      trim_left();
    }

    if (!source.good())
      return {};

    Location location = Location{source_path, row,
                                 static_cast<int>(source.tellg()) - line_begin};

    char c = source.peek();

    if (std::isalpha(c)) {
      const std::streampos start = source.tellg();
      while (source.good() && std::isalnum(source.peek()))
        chop_char();
      const std::streampos n = source.tellg() - start + 1;

      source.seekg(start);

      char *buf = new char[static_cast<int>(n) + 1];
      source.get(buf, n, '\0');

      std::string text = std::string(buf);

      delete[] buf;
      return Token{TokenType::IDENTITY, std::move(text), {}, location};
    }

    if (is_in_token_map(c)) {
      chop_char();
      return Token{token_type_map.at(c), {}, {}, location};
    }

    if (std::isdigit(c)) {
      const std::streampos start = source.tellg();
      while (source.good() && std::isdigit(source.peek()))
        chop_char();
      const std::streampos n = source.tellg() - start + 1;

      source.seekg(start);

      char *buf = new char[static_cast<int>(n) + 1];
      source.get(buf, n, '\0');

      int number = std::atoi(buf);

      delete[] buf;
      return Token{TokenType::NUMBER, {}, number, location};
    }

    if (c == '\"') {
      chop_char();
      const std::streampos start = source.tellg();
      while (source.good() && source.peek() != '\"')
        chop_char();
      const std::streampos n = source.tellg() - start + 1;

      source.seekg(start);

      char *buf = new char[static_cast<int>(n) + 1];
      source.get(buf, n, '\0');

      std::string text = std::string(buf);

      delete[] buf;
      chop_char();
      return Token{TokenType::STRING, text, {}, location};
    }

    assert(false && "Not implemented");
  }

  bool good() const { return source.good(); }
};

static void output_token(const Token &token) {
  std::cout << token.location.file_path << ":" << token.location.row + 1 << ":"
            << token.location.col + 1 << " ("
            << token_type_names[static_cast<int>(token.type)];
  if (token.text.has_value()) {
    if (token.type == TokenType::STRING)
      std::cout << ", \"" << token.text.value() << "\"";
    else
      std::cout << ", " << token.text.value();
  }
  if (token.number.has_value())
    std::cout << ", " << token.number.value();
  std::cout << ")" << std::endl;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <source.vcl>" << std::endl;
    std::cout << "No source file is provided." << std::endl;
    return 1;
  }

  Lexer lexer = Lexer(argv[1]);

  if (!lexer.good()) {
    std::cout << "Usage: " << argv[0] << " <source.vcl>" << std::endl;
    std::cout << "Source file " << argv[1] << " may not exist." << std::endl;
    return 1;
  }

  std::optional<Token> token = lexer.next_token();
  while (token) {
    output_token(token.value());
    token = lexer.next_token();
  }

  return 0;
}
