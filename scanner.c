#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slre.h"

#define MAX_TOKEN_LENGTH 1000
#define CAPS_SIZE 4  // slre 的 caps size

typedef enum {
  TOKEN_IDENTIFIER,     // IDEN
  TOKEN_RESERVED_WORD,  // REWD
  TOKEN_OPERATOR,       // OPER
  TOKEN_INTEGER,        // INTE
  TOKEN_STRING,         // STR
  TOKEN_FLOAT,          // FLOT
  TOKEN_CHARACTER,      // CHAR
  TOKEN_SPECIAL,        // SPEC
  TOKEN_PREPROCESSOR,   // PREP
  TOKEN_SINGLECOMMENT,  // SC
  TOKEN_MULTICOMMENT,   // MC
  TOKEN_EOF,
  TOKEN_ERROR
} TokenType;

typedef struct {
  TokenType type;
  char value[MAX_TOKEN_LENGTH];
  int line_number;
} Token;

// Scanner state
typedef struct {
  char* input;
  int position;
  int line_number;
  int length;
  FILE* output_file;
} Scanner;

// 保留字列表
const char* reserved_words[] = {
    "if",       "else",     "while",  "for",    "do",       "switch", "case",
    "default",  "continue", "int",    "long",   "float",    "double", "char",
    "break",    "static",   "extern", "auto",   "register", "sizeof", "union",
    "struct",   "short",    "enum",   "return", "goto",     "const",  "signed",
    "unsigned", "typedef",  "void"};
const int reserved_count = sizeof(reserved_words) / sizeof(reserved_words[0]);

// 運算子列表
const char* operators = "><=!+-*/%&|^~[],.";

// 特殊符號列表
const char* special_symbols = "{}();?:";

Scanner* create_scanner(const char* input, FILE* output_file);
void destroy_scanner(Scanner* scanner);
Token get_next_token(Scanner* scanner);
bool is_reserved_word(const char* str);
bool is_operator_char(char c);
bool is_special_char(char c);
void skip_whitespace(Scanner* scanner);
Token scan_identifier(Scanner* scanner);
Token scan_number(Scanner* scanner);
static void scan_literal_content(Scanner* scanner, char delimiter,
                                 char* buffer);
Token scan_string(Scanner* scanner);
Token scan_character(Scanner* scanner);
Token scan_operator(Scanner* scanner);
Token scan_special(Scanner* scanner);
Token scan_comment(Scanner* scanner);
Token scan_preprocessor(Scanner* scanner);
void print_token(Scanner* scanner, Token token, const char* comment_content);
char current_char(Scanner* scanner);
char peek_char(Scanner* scanner);
void advance(Scanner* scanner);

// Scanner main
// 因為 Unit Test 測試這個檔案的時候 scanner_test.cpp 的 main() 會和這個檔案的
// main() 衝突，造成 redefinition 所以 UNIT_TEST 時候不包含 main() 函式
#ifndef UNIT_TEST
int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: %s <input_file> <output_file>\n", argv[0]);
    return 1;
  }

  FILE* input_file = fopen(argv[1], "r");
  if (!input_file) {
    printf("Error: Cannot open input file %s\n", argv[1]);
    return 1;
  }

  // 取得檔案大小
  fseek(input_file, 0, SEEK_END);
  long file_size = ftell(input_file);
  fseek(input_file, 0, SEEK_SET);

  // Allocate memory and read file
  char* input = (char*)malloc(file_size + 1);
  fread(input, 1, file_size, input_file);
  input[file_size] = '\0';
  fclose(input_file);

  // Open output file
  FILE* output_file = fopen(argv[2], "w");
  if (!output_file) {
    printf("Error: Cannot create output file %s\n", argv[2]);
    free(input);
    return 1;
  }

  // Create scanner
  Scanner* scanner = create_scanner(input, output_file);

  // Scan all tokens
  Token token;
  do {
    token = get_next_token(scanner);
    if (token.type != TOKEN_EOF && token.type != TOKEN_ERROR) {
      if (token.type == TOKEN_SINGLECOMMENT) {
        print_token(scanner, token, token.value);
      } else if (token.type == TOKEN_MULTICOMMENT) {
        print_token(scanner, token, NULL);
      } else {
        print_token(scanner, token, NULL);
      }
    } else if (token.type == TOKEN_ERROR) {
      fprintf(scanner->output_file, "Error on line %d: %s\n", token.line_number,
              token.value);
    }
  } while (token.type != TOKEN_EOF);

  // Cleanup
  destroy_scanner(scanner);
  fclose(output_file);
  free(input);

  printf("Scanner completed. Output written to %s\n", argv[2]);
  return 0;
}
#endif

// 產生一個初始狀態的 Scanner state
Scanner* create_scanner(const char* input, FILE* output_file) {
  Scanner* scanner = (Scanner*)malloc(sizeof(Scanner));
  scanner->input = (char*)input;
  scanner->position = 0;
  scanner->line_number = 1;
  scanner->length = strlen(input);
  scanner->output_file = output_file;
  return scanner;
}

void destroy_scanner(Scanner* scanner) { free(scanner); }

// Get current character
char current_char(Scanner* scanner) {
  if (scanner->position >= scanner->length) {
    return '\0';
  }
  return scanner->input[scanner->position];
}

// 偷看下一個 char (不會改變 scanner 的 position)
char peek_char(Scanner* scanner) {
  if (scanner->position + 1 >= scanner->length) {
    return '\0';
  }
  return scanner->input[scanner->position + 1];
}

// 往前移動 scanner 的 position
void advance(Scanner* scanner) {
  if (scanner->position < scanner->length) {
    if (scanner->input[scanner->position] == '\n') {
      scanner->line_number++;
    }
    scanner->position++;
  }
}

void skip_whitespace(Scanner* scanner) {
  while (current_char(scanner) != '\0' && isspace(current_char(scanner))) {
    advance(scanner);
  }
}

bool is_reserved_word(const char* str) {
  for (int i = 0; i < reserved_count; i++) {
    if (strcmp(str, reserved_words[i]) == 0) {
      return true;
    }
  }
  return false;
}

bool is_operator_char(char c) { return strchr(operators, c) != NULL; }

bool is_special_char(char c) { return strchr(special_symbols, c) != NULL; }

// Scan identifier or reserved word
Token scan_identifier(Scanner* scanner) {
  Token token;
  token.line_number = scanner->line_number;
  int i = 0;

  // Identifiers 必須以字母或 '_' 開頭
  if (isalpha(current_char(scanner)) || current_char(scanner) == '_') {
    token.value[i++] = current_char(scanner);
    advance(scanner);

    while (current_char(scanner) != '\0' &&
           (isalnum(current_char(scanner)) || current_char(scanner) == '_')) {
      if (i < MAX_TOKEN_LENGTH - 1) {
        token.value[i++] = current_char(scanner);
      }
      advance(scanner);
    }
  }

  token.value[i] = '\0';

  if (is_reserved_word(token.value)) {
    token.type = TOKEN_RESERVED_WORD;
  } else {
    token.type = TOKEN_IDENTIFIER;
  }

  return token;
}

Token scan_number(Scanner* scanner) {
  Token token;
  token.line_number = scanner->line_number;
  int i = 0;

  while (current_char(scanner) != '\0' &&
         (isdigit(current_char(scanner)) || current_char(scanner) == '.' ||
          current_char(scanner) == 'e' || current_char(scanner) == 'E' ||
          current_char(scanner) == '+' || current_char(scanner) == '-')) {
    if (i < MAX_TOKEN_LENGTH - 1) {
      token.value[i++] = current_char(scanner);
    }
    advance(scanner);
  }
  token.value[i] = '\0';

  struct slre_cap caps[CAPS_SIZE];

  // 判斷是否為整數
  const int r_int = slre_match("^\\d+$", token.value, strlen(token.value), caps,
                               CAPS_SIZE, 0);
  if (r_int > 0) {
    token.type = TOKEN_INTEGER;
    return token;
  }

  // 判斷是否為浮點數
  const int r_float =
      slre_match("^(\\d*\\.\\d+|\\d+\\.\\d*)([Ee][+|-]?\\d+)?$", token.value,
                 strlen(token.value), caps, CAPS_SIZE, 0);
  if (r_float > 0) {
    token.type = TOKEN_FLOAT;
    return token;
  }

  // 如果都不是，則視為錯誤
  token.type = TOKEN_ERROR;
  snprintf(token.value, sizeof(token.value), "Invalid number: %s", token.value);
  return token;
}

static void scan_literal_content(Scanner* scanner, char delimiter,
                                 char* buffer) {
  int i = 0;
  // Skip 開始的 delimiter
  advance(scanner);

  while (current_char(scanner) != '\0' && current_char(scanner) != delimiter) {
    if (current_char(scanner) == '\\') {
      // Handle escape sequences
      advance(scanner);
      if (current_char(scanner) != '\0') {
        if (i < MAX_TOKEN_LENGTH - 1) {
          buffer[i++] = '\\';
        }
        if (i < MAX_TOKEN_LENGTH - 1) {
          buffer[i++] = current_char(scanner);
        }
        advance(scanner);
      }
    } else {
      if (i < MAX_TOKEN_LENGTH - 1) {
        buffer[i++] = current_char(scanner);
      }
      advance(scanner);
    }
  }

  // Skip 結束的 delimiter
  if (current_char(scanner) == delimiter) {
    advance(scanner);
  }

  buffer[i] = '\0';
}

Token scan_string(Scanner* scanner) {
  Token token;
  token.type = TOKEN_STRING;
  token.line_number = scanner->line_number;
  scan_literal_content(scanner, '"', token.value);
  return token;
}

Token scan_character(Scanner* scanner) {
  Token token;
  token.type = TOKEN_CHARACTER;
  token.line_number = scanner->line_number;
  scan_literal_content(scanner, '\'', token.value);
  return token;
}

Token scan_operator(Scanner* scanner) {
  Token token;
  token.type = TOKEN_OPERATOR;
  token.line_number = scanner->line_number;
  int i = 0;

  char first = current_char(scanner);
  char second = peek_char(scanner);

  // 處理 ++ -- += -= *= /= %= <<= >>= == != && || -> 等運算子
  if ((first == '=' && second == '=') || (first == '&' && second == '&') ||
      (first == '|' && second == '|') || (first == '+' && second == '+') ||
      (first == '-' && second == '-') || (first == '+' && second == '=') ||
      (first == '-' && second == '=') || (first == '*' && second == '=') ||
      (first == '/' && second == '=') || (first == '%' && second == '=') ||
      (first == '<' && second == '<') || (first == '>' && second == '>') ||
      (first == '<' && second == '=') || (first == '>' && second == '=') ||
      (first == '!' && second == '=') || (first == '-' && second == '>')) {
    token.value[i++] = first;
    advance(scanner);
    token.value[i++] = current_char(scanner);
    advance(scanner);
  } else {
    token.value[i++] = current_char(scanner);
    advance(scanner);
  }

  token.value[i] = '\0';
  return token;
}

Token scan_special(Scanner* scanner) {
  Token token;
  token.type = TOKEN_SPECIAL;
  token.line_number = scanner->line_number;

  token.value[0] = current_char(scanner);
  token.value[1] = '\0';
  advance(scanner);

  return token;
}

Token scan_comment(Scanner* scanner) {
  Token token;
  token.line_number = scanner->line_number;
  int i = 0;

  // 單行註解
  if (current_char(scanner) == '/' && peek_char(scanner) == '/') {
    token.type = TOKEN_SINGLECOMMENT;

    // 儲存註解內容
    while (current_char(scanner) != '\0' && current_char(scanner) != '\n') {
      if (i < MAX_TOKEN_LENGTH - 1) {
        token.value[i++] = current_char(scanner);
      }
      advance(scanner);
    }
    token.value[i] = '\0';

  } else if (current_char(scanner) == '/' && peek_char(scanner) == '*') {
    // 多行註解
    token.type = TOKEN_MULTICOMMENT;
    int start_line = scanner->line_number;

    advance(scanner);  // skip '/'
    advance(scanner);  // skip '*'

    while (current_char(scanner) != '\0') {
      if (current_char(scanner) == '*' && peek_char(scanner) == '/') {
        advance(scanner);  // skip '*'
        advance(scanner);  // skip '/'
        break;
      }
      advance(scanner);
    }

    snprintf(token.value, sizeof(token.value), "%d-%d", start_line,
             scanner->line_number);
  }

  return token;
}

Token scan_preprocessor(Scanner* scanner) {
  Token token;
  token.type = TOKEN_PREPROCESSOR;
  token.line_number = scanner->line_number;
  int i = 0;

  while (current_char(scanner) != '\0' && current_char(scanner) != '\n') {
    if (i < MAX_TOKEN_LENGTH - 1) {
      token.value[i++] = current_char(scanner);
    }
    advance(scanner);
  }

  token.value[i] = '\0';
  return token;
}

Token get_next_token(Scanner* scanner) {
  skip_whitespace(scanner);

  Token token;
  char c = current_char(scanner);

  if (c == '\0') {
    token.type = TOKEN_EOF;
    token.line_number = scanner->line_number;
    strcpy(token.value, "EOF");
    return token;
  }

  // Comments
  if (c == '/' && (peek_char(scanner) == '/' || peek_char(scanner) == '*')) {
    return scan_comment(scanner);
  }

  // Preprocessor directives
  if (c == '#') {
    return scan_preprocessor(scanner);
  }

  // String literals
  if (c == '"') {
    return scan_string(scanner);
  }

  // Character literals
  if (c == '\'') {
    return scan_character(scanner);
  }

  // Numbers
  // 特殊情況：.3
  if (isdigit(c) || (c == '.' && isdigit(peek_char(scanner)))) {
    return scan_number(scanner);
  }

  // Identifiers and reserved_words
  if (isalpha(c) || c == '_') {
    return scan_identifier(scanner);
  }

  // Operators
  if (is_operator_char(c)) {
    return scan_operator(scanner);
  }

  // Special characters
  if (is_special_char(c)) {
    return scan_special(scanner);
  }

  // Error - unrecognized character
  token.type = TOKEN_ERROR;
  token.line_number = scanner->line_number;
  snprintf(token.value, sizeof(token.value), "Unrecognized character: %c", c);
  advance(scanner);
  return token;
}

// Print token to output file
void print_token(Scanner* scanner, Token token, const char* comment_content) {
  const char* type_str;

  switch (token.type) {
    case TOKEN_IDENTIFIER:
      type_str = "IDEN";
      break;
    case TOKEN_RESERVED_WORD:
      type_str = "REWD";
      break;
    case TOKEN_OPERATOR:
      type_str = "OPER";
      break;
    case TOKEN_INTEGER:
      type_str = "INTE";
      break;
    case TOKEN_FLOAT:
      type_str = "FLOT";
      break;
    case TOKEN_STRING:
      type_str = "STR";
      break;
    case TOKEN_CHARACTER:
      type_str = "CHAR";
      break;
    case TOKEN_SPECIAL:
      type_str = "SPEC";
      break;
    case TOKEN_PREPROCESSOR:
      type_str = "PREP";
      break;
    case TOKEN_SINGLECOMMENT:
      type_str = "SC";
      break;
    case TOKEN_MULTICOMMENT:
      type_str = "MC";
      fprintf(scanner->output_file, "%s %s\n", token.value, type_str);
      return;
    default:
      return;
  }

  fprintf(scanner->output_file, "%d %s %s\n", token.line_number, type_str,
          token.value);
}