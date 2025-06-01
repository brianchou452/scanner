// Google Test for scanner.c
extern "C" {
#include "../scanner.c"
}
#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#define NULL_DEVICE "NUL"
#else
#define NULL_DEVICE "/dev/null"
#endif

// Helper to create a scanner from string
Scanner* make_scanner_from_str(const char* str) {
  FILE* dummy = fopen(NULL_DEVICE, "w");
  Scanner* s = create_scanner(str, dummy);
  fclose(dummy);
  return s;
}

TEST(ScannerTest, CreateAndDestroyScanner) {
  const char* input = "int a = 5;";
  FILE* dummy = fopen(NULL_DEVICE, "w");
  Scanner* scanner = create_scanner(input, dummy);
  ASSERT_NE(scanner, nullptr);
  destroy_scanner(scanner);
  fclose(dummy);
}

TEST(ScannerTest, IsReservedWord) {
  EXPECT_TRUE(is_reserved_word("int"));
  EXPECT_FALSE(is_reserved_word("foo"));
}

TEST(ScannerTest, IsOperatorChar) {
  EXPECT_TRUE(is_operator_char('+'));
  EXPECT_FALSE(is_operator_char('a'));
}

TEST(ScannerTest, IsSpecialChar) {
  EXPECT_TRUE(is_special_char(';'));
  EXPECT_FALSE(is_special_char('a'));
}

TEST(ScannerTest, ScanIdentifier) {
  Scanner* s = make_scanner_from_str("foo123 ");
  Token t = scan_identifier(s);
  EXPECT_EQ(t.type, TOKEN_IDENTIFIER);
  EXPECT_STREQ(t.value, "foo123");
  destroy_scanner(s);
}

TEST(ScannerTest, ScanNumber) {
  std::string test_input[] = {
      "12345",  "0",       ".14",     "0.001",   "2.71828e10",
      "1.0E-5", "3.14e+2", "1.0e-10", "1.0E+10", "1.",
  };
  TokenType expected_output[] = {
      TOKEN_INTEGER, TOKEN_INTEGER, TOKEN_FLOAT, TOKEN_FLOAT, TOKEN_FLOAT,
      TOKEN_FLOAT,   TOKEN_FLOAT,   TOKEN_FLOAT, TOKEN_FLOAT, TOKEN_FLOAT,
  };
  for (auto i = 0; i < sizeof(test_input) / sizeof(test_input[0]); ++i) {
    Scanner* s = make_scanner_from_str(test_input[i].c_str());
    Token t = scan_number(s);
    EXPECT_EQ(t.type, expected_output[i]);
    EXPECT_STREQ(t.value, test_input[i].c_str());
    destroy_scanner(s);
  }
}

TEST(ScannerTest, ScanString) {
  std::string test_input[] = {
      "\"hello\"",
      "\"hello world\"",
  };
  std::string expected_output[] = {
      "hello",
      "hello world",
  };
  for (auto i = 0; i < sizeof(test_input) / sizeof(test_input[0]); ++i) {
    Scanner* s = make_scanner_from_str(test_input[i].c_str());
    Token t = scan_string(s);
    EXPECT_EQ(t.type, TOKEN_STRING);
    EXPECT_STREQ(t.value, expected_output[i].c_str());
    destroy_scanner(s);
  }
}

TEST(ScannerTest, ScanCharacter) {
  Scanner* s = make_scanner_from_str("'a'");
  Token t = scan_character(s);
  EXPECT_EQ(t.type, TOKEN_CHARACTER);
  EXPECT_STREQ(t.value, "a");
  destroy_scanner(s);
}

// 讀入 ScanCharacter_in.txt 與 ScanCharacter_out.txt，逐行測試 scan_character
TEST(ScannerTest, ScanCharacterFromFile) {
  std::ifstream infile("../test/data/ScanCharacter_in.txt");
  std::ifstream outfile("../test/data/ScanCharacter_out.txt");
  ASSERT_TRUE(infile.is_open());
  ASSERT_TRUE(outfile.is_open());
  std::string input_line, output_line;
  int line_num = 1;
  while (std::getline(infile, input_line) &&
         std::getline(outfile, output_line)) {
    Scanner* s = make_scanner_from_str(input_line.c_str());
    Token t = scan_character(s);
    EXPECT_EQ(t.type, TOKEN_CHARACTER) << "at line " << line_num;
    EXPECT_STREQ(t.value, output_line.c_str()) << "at line " << line_num;
    destroy_scanner(s);
    ++line_num;
  }
  // 檢查兩檔案行數是否一致
  EXPECT_FALSE(std::getline(infile, input_line))
      << "Input file has more lines than output file";
  EXPECT_FALSE(std::getline(outfile, output_line))
      << "Output file has more lines than input file";
}

TEST(ScannerTest, ScanOperator) {
  Scanner* s = make_scanner_from_str("++");
  Token t = scan_operator(s);
  EXPECT_EQ(t.type, TOKEN_OPERATOR);
  EXPECT_STREQ(t.value, "++");
  destroy_scanner(s);
}

TEST(ScannerTest, ScanSpecial) {
  Scanner* s = make_scanner_from_str(";");
  Token t = scan_special(s);
  EXPECT_EQ(t.type, TOKEN_SPECIAL);
  EXPECT_STREQ(t.value, ";");
  destroy_scanner(s);
}

TEST(ScannerTest, ScanCommentSingle) {
  Scanner* s = make_scanner_from_str("// comment\n");
  Token t = scan_comment(s);
  EXPECT_EQ(t.type, TOKEN_SINGLECOMMENT);
  EXPECT_STREQ(t.value, "// comment");
  destroy_scanner(s);
}

TEST(ScannerTest, ScanCommentMulti) {
  Scanner* s = make_scanner_from_str("/* multi\ncomment */");
  Token t = scan_comment(s);
  EXPECT_EQ(t.type, TOKEN_MULTICOMMENT);
  // value is line range, e.g. "1-2"
  EXPECT_TRUE(strchr(t.value, '-'));
  destroy_scanner(s);
}

TEST(ScannerTest, ScanPreprocessor) {
  Scanner* s = make_scanner_from_str("#include <stdio.h>\n");
  Token t = scan_preprocessor(s);
  EXPECT_EQ(t.type, TOKEN_PREPROCESSOR);
  EXPECT_STREQ(t.value, "#include <stdio.h>");
  destroy_scanner(s);
}

TEST(ScannerTest, GetNextTokenEOF) {
  Scanner* s = make_scanner_from_str("");
  Token t = get_next_token(s);
  EXPECT_EQ(t.type, TOKEN_EOF);
  destroy_scanner(s);
}

TEST(ScannerTest, GetNextTokenIdentifier) {
  Scanner* s = make_scanner_from_str("foo");
  Token t = get_next_token(s);
  EXPECT_EQ(t.type, TOKEN_IDENTIFIER);
  EXPECT_STREQ(t.value, "foo");
  destroy_scanner(s);
}

TEST(ScannerTest, GetNextTokenNumber) {
  Scanner* s = make_scanner_from_str("123");
  Token t = get_next_token(s);
  EXPECT_EQ(t.type, TOKEN_INTEGER);
  EXPECT_STREQ(t.value, "123");
  destroy_scanner(s);
}

// main for gtest
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
