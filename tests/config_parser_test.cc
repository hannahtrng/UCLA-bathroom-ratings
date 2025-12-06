#include "gtest/gtest.h"
#include "config_parser.h"
class ParserTest: public ::testing::Test {
  protected:
    NginxConfigParser parser;
    NginxConfig config;
};
// Test Config
TEST(NginxConfigParserTest, SimpleConfig) {
  NginxConfigParser parser;
  NginxConfig out_config;
  bool success = parser.Parse("example_config", &out_config);
  EXPECT_TRUE(success);
}
// Test fixture Config
TEST_F(ParserTest, SimpleConfigFixture) {
   bool success = parser.Parse("example_config", &config);
  EXPECT_TRUE(success);
}
// Test comments (should succeed)
TEST_F(ParserTest, ConfigWithComments) {
  bool success = parser.Parse("example_config2", &config);
  EXPECT_TRUE(success);
}
// Test single double quote (should fail (expect false should succeed))
TEST_F(ParserTest, ConfigWithDoubleQuote) {
  bool success = parser.Parse("example_config3", &config);
  EXPECT_FALSE(success);
}
// Test single single quote (should fail)
TEST_F(ParserTest, ConfigWithSingleQuote) {
  bool success = parser.Parse("example_config4", &config);
  EXPECT_FALSE(success);
}
// Test no white space after quote (should fail)
TEST_F(ParserTest, ConfigWithNoWhiteSpace) {
  bool success = parser.Parse("example_config5", &config);
  EXPECT_FALSE(success);
}
// Test empty config file
TEST_F(ParserTest, EmptyFile){
  bool success = parser.Parse("example_config6", &config);
  EXPECT_FALSE(success);
}
// Test nested config
TEST_F(ParserTest, NestedConfig){
  bool success = parser.Parse("example_config7", &config);
  EXPECT_TRUE(success);
}
// Test tabs and white space
TEST_F(ParserTest, ConfigWithTabs) {
  bool success = parser.Parse("example_config8", &config);
  EXPECT_TRUE(success);
}
// Test unclosed open bracket
TEST_F(ParserTest, UnclosedOpenBracket) {
  bool success = parser.Parse("example_config9", &config);
  EXPECT_FALSE(success);
}
// Test unclosed closed bracket
TEST_F(ParserTest, UnclosedClosedBracket) {
  bool success = parser.Parse("example_config10", &config);
  EXPECT_FALSE(success);
}
// Test double quote backslash escape
TEST_F(ParserTest, DoubleQuoteBackslashEscape) {
  bool success = parser.Parse("example_config11", &config);
  EXPECT_TRUE(success);
}
// Test single quote backslash escape
TEST_F(ParserTest, SingleQuoteBackslashEscape) {
  bool success = parser.Parse("example_config12", &config);
  EXPECT_TRUE(success);
}

// created these tests with the assistance of ai!
TEST_F(ParserTest, ToString_SimpleAndNestedBlocks_UsesIndentation) {
  std::istringstream in(
      "user www-data;\n"
      "http {\n"
      "  include mime.types;\n"
      "}\n"
  );
  NginxConfig local; 
  ASSERT_TRUE(parser.Parse(&in, &local));

  const std::string got = local.ToString(0);
  const std::string expected =
      "user www-data;\n"
      "http {\n"
      "  include mime.types;\n"
      "}\n";
  EXPECT_EQ(got, expected);
}

TEST_F(ParserTest, ToString_ManualConstruction_InnerIndentLoopHit) {
  NginxConfig local;

  auto st = std::make_unique<NginxConfigStatement>();
  st->tokens_.push_back("events");
  st->child_block_ = std::make_unique<NginxConfig>();
  {
    auto inner = std::make_unique<NginxConfigStatement>();
    inner->tokens_.push_back("worker_connections");
    inner->tokens_.push_back("1024");
    st->child_block_->statements_.emplace_back(std::move(inner));
  }
  local.statements_.emplace_back(std::move(st));

  const std::string s = local.ToString(0);
  EXPECT_NE(s.find("events {\n"), std::string::npos);
  EXPECT_NE(s.find("  worker_connections 1024;\n"), std::string::npos);
  EXPECT_NE(s.find("}\n"), std::string::npos);
}

TEST_F(ParserTest, Quotes_DoubleQuoteEscapesAreHandled) {
  std::istringstream in("name \"a\\\"b\";");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Quotes_SingleQuoteEscapesAreHandled) {
  std::istringstream in("name 'a\\'b';");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Quotes_DoubleQuoteFollowedByBadCharIsError) {
  std::istringstream in("name \"ok\"x;");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Quotes_SingleQuoteFollowedByBadCharIsError) {
  std::istringstream in("name 'ok'x;");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Error_StraySemicolonOnly) {
  std::istringstream in(";");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Error_ExtraClosingBraceAtTopLevel) {
  std::istringstream in("}");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Error_StartBlockAfterStatementEndIsInvalid) {
  std::istringstream in("foo; { }\n");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Error_EofAfterNormalTokenIsInvalid) {
  std::istringstream in("foo");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Error_FileOpenFailureReturnsFalse) {
  NginxConfig local;
  EXPECT_FALSE(parser.Parse("this_file_should_not_exist_12345", &local));
}


TEST_F(ParserTest, Quotes_DoubleQuoteFollowedByCloseBrace_Error) {
  std::istringstream in("name \"ok\"}\n");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Quotes_DoubleQuoteFollowedByTab_OK) {
  std::istringstream in("name \"ok\"\t;\n");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Quotes_DoubleQuoteFollowedByNewline_OK) {
  std::istringstream in("name \"ok\"\n;\n");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, Quotes_SingleQuoteFollowedByCloseBrace_Error) {
  std::istringstream in("name 'ok'}\n");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, NormalToken_FollowedByCloseBrace_NoSpace_Error) {
  std::istringstream in("name}\n");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}


// Comment state ends on '\r' as well as '\n'.
TEST_F(ParserTest, Comment_TerminatedByCarriageReturn_OK) {
  std::istringstream in(
      "# this is a comment\r"
      "user www-data;\n"
  );
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
}


TEST_F(ParserTest, NormalToken_FollowedByOpenBrace_NoSpace_OK_NonQuoted) {
  std::istringstream in("events{ worker_connections 1024; }\n");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, NormalToken_FollowedByCloseBrace_NoSpace_Error_NonQuoted) {
  std::istringstream in("name foo}\n");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}


TEST_F(ParserTest, Error_StartBlockImmediatelyAfterEndBlock_IsInvalid) {
  std::istringstream in("{ a b; }{ c d; }\n");
  NginxConfig local;
  EXPECT_FALSE(parser.Parse(&in, &local));
}

TEST_F(ParserTest, ToString_NestedPrintedAtDepthTwo) {
  NginxConfig root;
  auto st = std::make_unique<NginxConfigStatement>();
  st->tokens_.push_back("server");
  st->child_block_ = std::make_unique<NginxConfig>();
  {
    auto inner = std::make_unique<NginxConfigStatement>();
    inner->tokens_.push_back("listen");
    inner->tokens_.push_back("8080");
    st->child_block_->statements_.emplace_back(std::move(inner));
  }
  root.statements_.emplace_back(std::move(st));

  const std::string printed = root.ToString(/*depth=*/2);
  EXPECT_NE(printed.find("    server {\n"), std::string::npos);     
  EXPECT_NE(printed.find("      listen 8080;\n"), std::string::npos); 
  EXPECT_NE(printed.find("    }\n"), std::string::npos);
}

// Helper class to test protected methods
class TestParser : public NginxConfigParser {
public:
  using NginxConfigParser::TokenTypeAsString;
  using NginxConfigParser::TokenType;
  using NginxConfigParser::TOKEN_TYPE_COMMENT;
  using NginxConfigParser::TOKEN_TYPE_QUOTED_STRING;
};

// Test TokenTypeAsString with TOKEN_TYPE_COMMENT (targets line 59)
TEST(TokenTypeAsStringTest, CommentType) {
  TestParser parser;
  const char* result = parser.TokenTypeAsString(TestParser::TOKEN_TYPE_COMMENT);
  EXPECT_STREQ(result, "TOKEN_TYPE_COMMENT");
}

// Test TokenTypeAsString with TOKEN_TYPE_QUOTED_STRING (targets line 63)
TEST(TokenTypeAsStringTest, QuotedStringType) {
  TestParser parser;
  const char* result = parser.TokenTypeAsString(TestParser::TOKEN_TYPE_QUOTED_STRING);
  EXPECT_STREQ(result, "TOKEN_TYPE_QUOTED_STRING");
}

// Test TokenTypeAsString with invalid token type (targets line 64 - default case)
TEST(TokenTypeAsStringTest, InvalidType) {
  TestParser parser;
  // Cast an invalid int to TokenType to hit the default case
  const char* result = parser.TokenTypeAsString(static_cast<TestParser::TokenType>(999));
  EXPECT_STREQ(result, "Unknown token type");
}

// Test END_BLOCK followed by NORMAL token (targets line 211)
TEST_F(ParserTest, EndBlockFollowedByNormal) {
  std::istringstream in("server { listen 80; } location /;\n");
  NginxConfig local;
  // This should succeed - a normal token after } is valid (starts new statement)
  EXPECT_TRUE(parser.Parse(&in, &local));
  ASSERT_EQ(local.statements_.size(), 2);
  EXPECT_EQ(local.statements_[1]->tokens_[0], "location");
}

// Test multiple NORMAL tokens in sequence (targets line 212)
TEST_F(ParserTest, MultipleNormalTokens) {
  std::istringstream in("directive token1 token2 token3;\n");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
  ASSERT_EQ(local.statements_.size(), 1);
  ASSERT_EQ(local.statements_[0]->tokens_.size(), 4);
  EXPECT_EQ(local.statements_[0]->tokens_[0], "directive");
  EXPECT_EQ(local.statements_[0]->tokens_[1], "token1");
  EXPECT_EQ(local.statements_[0]->tokens_[2], "token2");
  EXPECT_EQ(local.statements_[0]->tokens_[3], "token3");
}

// Test many consecutive NORMAL tokens (ensures line 212 branch is hit)
TEST_F(ParserTest, ManyConsecutiveNormalTokens) {
  std::istringstream in("cmd arg1 arg2 arg3 arg4 arg5 arg6 arg7;\n");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
  ASSERT_EQ(local.statements_.size(), 1);
  EXPECT_EQ(local.statements_[0]->tokens_.size(), 8);
}

// Test QUOTED_STRING followed by semicolon (line 225 branch 3)
TEST_F(ParserTest, QuotedStringFollowedBySemicolon) {
  std::istringstream in("name \"value\";\n");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
  ASSERT_EQ(local.statements_.size(), 1);
  EXPECT_EQ(local.statements_[0]->tokens_[1], "\"value\"");
}

// Test QUOTED_STRING followed by START_BLOCK (line 231 branch 3)
TEST_F(ParserTest, QuotedStringFollowedByBlock) {
  std::istringstream in("directive \"value\" { nested arg; }\n");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
  ASSERT_EQ(local.statements_.size(), 1);
  EXPECT_EQ(local.statements_[0]->tokens_[1], "\"value\"");
  EXPECT_NE(local.statements_[0]->child_block_.get(), nullptr);
}

// Test single quoted string followed by semicolon
TEST_F(ParserTest, SingleQuotedStringFollowedBySemicolon) {
  std::istringstream in("name 'value';\n");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
  ASSERT_EQ(local.statements_.size(), 1);
  EXPECT_EQ(local.statements_[0]->tokens_[1], "'value'");
}

// Test single quoted string followed by block
TEST_F(ParserTest, SingleQuotedStringFollowedByBlock) {
  std::istringstream in("directive 'value' { nested arg; }\n");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
  ASSERT_EQ(local.statements_.size(), 1);
  EXPECT_NE(local.statements_[0]->child_block_.get(), nullptr);
}

// Test END_BLOCK followed by END_BLOCK (nested blocks)
TEST_F(ParserTest, EndBlockFollowedByEndBlock) {
  std::istringstream in(
    "outer {\n"
    "  inner {\n"
    "    statement arg;\n"
    "  }\n"
    "}\n"
  );
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
  ASSERT_EQ(local.statements_.size(), 1);
  EXPECT_NE(local.statements_[0]->child_block_.get(), nullptr);
}

// Test statement with 10+ tokens
TEST_F(ParserTest, VeryManyTokensInStatement) {
  std::istringstream in("cmd t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 t11 t12;\n");
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
  ASSERT_EQ(local.statements_.size(), 1);
  EXPECT_EQ(local.statements_[0]->tokens_.size(), 13);
}

// Test deeply nested blocks (3 levels)
TEST_F(ParserTest, ThreeLevelNesting) {
  std::istringstream in(
    "level1 {\n"
    "  level2 {\n"
    "    level3 {\n"
    "      stmt val;\n"
    "    }\n"
    "  }\n"
    "}\n"
  );
  NginxConfig local;
  EXPECT_TRUE(parser.Parse(&in, &local));
  ASSERT_EQ(local.statements_.size(), 1);
  auto l1 = local.statements_[0]->child_block_.get();
  ASSERT_NE(l1, nullptr);
  ASSERT_EQ(l1->statements_.size(), 1);
  auto l2 = l1->statements_[0]->child_block_.get();
  ASSERT_NE(l2, nullptr);
}