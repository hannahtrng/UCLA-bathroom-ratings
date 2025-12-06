// An nginx config file parser.
//
// See:
//   http://wiki.nginx.org/Configuration
//   http://blog.martinfjordvald.com/2010/07/nginx-primer/
//
// How Nginx does it:
//   http://lxr.nginx.org/source/src/core/ngx_conf_file.c

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "config_parser.h"

#include <boost/log/trivial.hpp>

std::string NginxConfig::ToString(int depth) {
  std::string serialized_config;
  for (const auto& statement : statements_) {
    serialized_config.append(statement->ToString(depth));
  }
  return serialized_config;
}

std::string NginxConfigStatement::ToString(int depth) {
  std::string serialized_statement;
  for (int i = 0; i < depth; ++i) {
    serialized_statement.append("  ");
  }
  for (unsigned int i = 0; i < tokens_.size(); ++i) {
    if (i != 0) {
      serialized_statement.append(" ");
    }
    serialized_statement.append(tokens_[i]);
  }
  if (child_block_.get() != nullptr) {
    serialized_statement.append(" {\n");
    serialized_statement.append(child_block_->ToString(depth + 1));
    for (int i = 0; i < depth; ++i) {
      serialized_statement.append("  ");
    }
    serialized_statement.append("}");
  } else {
    serialized_statement.append(";");
  }
  serialized_statement.append("\n");
  return serialized_statement;
}

const char* NginxConfigParser::TokenTypeAsString(TokenType type) {
  switch (type) {
    case TOKEN_TYPE_START:         return "TOKEN_TYPE_START";
    case TOKEN_TYPE_NORMAL:        return "TOKEN_TYPE_NORMAL";
    case TOKEN_TYPE_START_BLOCK:   return "TOKEN_TYPE_START_BLOCK";
    case TOKEN_TYPE_END_BLOCK:     return "TOKEN_TYPE_END_BLOCK";
    case TOKEN_TYPE_COMMENT:       return "TOKEN_TYPE_COMMENT";
    case TOKEN_TYPE_STATEMENT_END: return "TOKEN_TYPE_STATEMENT_END";
    case TOKEN_TYPE_EOF:           return "TOKEN_TYPE_EOF";
    case TOKEN_TYPE_ERROR:         return "TOKEN_TYPE_ERROR";
    case TOKEN_TYPE_QUOTED_STRING:  return "TOKEN_TYPE_QUOTED_STRING";
    default:                       return "Unknown token type";
  }
}

// Helper: Check if character is a token delimiter (also valid after quotes)
bool NginxConfigParser::IsTokenDelimiter(char c) {
  return c == ' ' || c == '\t' || c == '\n' || 
         c == ';' || c == '{' || c == '}';
}

// Helper: Check if character is whitespace
bool NginxConfigParser::IsWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Helper: Check if a token type can be followed by STATEMENT_END or START_BLOCK
bool NginxConfigParser::IsValidTokenPredecessor(TokenType type) {
  return type == TOKEN_TYPE_NORMAL || type == TOKEN_TYPE_QUOTED_STRING;
}

// Helper: Check if a token type can be followed by NORMAL token
bool NginxConfigParser::CanFollowWithToken(TokenType type) {
  return type == TOKEN_TYPE_START ||
         type == TOKEN_TYPE_STATEMENT_END ||
         type == TOKEN_TYPE_START_BLOCK ||
         type == TOKEN_TYPE_END_BLOCK ||
         type == TOKEN_TYPE_NORMAL ||
         type == TOKEN_TYPE_QUOTED_STRING;
}

NginxConfigParser::TokenType NginxConfigParser::ParseToken(std::istream* input,
                                                           std::string* value) {
  TokenParserState state = TOKEN_STATE_INITIAL_WHITESPACE;
  while (input->good()) {
    const char c = input->get();
    if (!input->good()) {
      break;
    }
    switch (state) {
      case TOKEN_STATE_INITIAL_WHITESPACE:
        if (IsWhitespace(c)) {
          continue;
        }
        switch (c) {
          case '{':
            *value = c;
            return TOKEN_TYPE_START_BLOCK;
          case '}':
            *value = c;
            return TOKEN_TYPE_END_BLOCK;
          case '#':
            *value = c;
            state = TOKEN_STATE_TOKEN_TYPE_COMMENT;
            continue;
          case '"':
            *value = c;
            state = TOKEN_STATE_DOUBLE_QUOTE;
            continue;
          case '\'':
            *value = c;
            state = TOKEN_STATE_SINGLE_QUOTE;
            continue;
          case ';':
            *value = c;
            return TOKEN_TYPE_STATEMENT_END;
          default:
            *value += c;
            state = TOKEN_STATE_TOKEN_TYPE_NORMAL;
            continue;
        }
      case TOKEN_STATE_SINGLE_QUOTE:
        if(c == '\\') {
          *value += c;
          char next = input->get();
          *value += next;
          continue;
        }
        *value += c;
        if (c == '\'') {
          char next = input->get();
          if (IsTokenDelimiter(next)) {
              input->unget();
              return TOKEN_TYPE_NORMAL;
          } else {
            return TOKEN_TYPE_ERROR;
          }
        }
        continue;
      case TOKEN_STATE_DOUBLE_QUOTE:
      if(c == '\\') {
          *value += c;
          char next = input->get();
          *value += next;
          continue;
        }

        *value += c;
        if (c == '"') {
          char next = input->get();
          if (IsTokenDelimiter(next)) {
              input->unget();
              return TOKEN_TYPE_NORMAL;
          } else {
            return TOKEN_TYPE_ERROR;
          }
        }
        continue;
      case TOKEN_STATE_TOKEN_TYPE_COMMENT:
        if (c == '\n' || c == '\r') {
          return TOKEN_TYPE_COMMENT;
        }
        *value += c;
        continue;
      case TOKEN_STATE_TOKEN_TYPE_NORMAL:
        if (IsTokenDelimiter(c)) {
          input->unget();
          return TOKEN_TYPE_NORMAL;
        }
        *value += c;
        continue;
    }
  }

  // If we get here, we reached the end of the file.
  if (state == TOKEN_STATE_SINGLE_QUOTE ||
      state == TOKEN_STATE_DOUBLE_QUOTE) {
    return TOKEN_TYPE_ERROR;
  }

  return TOKEN_TYPE_EOF;
}

bool NginxConfigParser::Parse(std::istream* config_file, NginxConfig* config) {
  std::stack<NginxConfig*> config_stack;
  config_stack.push(config);
  TokenType last_token_type = TOKEN_TYPE_START;
  TokenType token_type;
  while (true) {
    std::string token;
    token_type = ParseToken(config_file, &token);
    if (token_type == TOKEN_TYPE_ERROR) {
      break;
    }

    if (token_type == TOKEN_TYPE_COMMENT) {
      // Skip comments.
      continue;
    }

    if (token_type == TOKEN_TYPE_START) {
      // Error.
      break;
    } else if (token_type == TOKEN_TYPE_NORMAL) {
      if (CanFollowWithToken(last_token_type)) {
        if (last_token_type != TOKEN_TYPE_NORMAL) {
          config_stack.top()->statements_.emplace_back(
              new NginxConfigStatement);
        }
        config_stack.top()->statements_.back().get()->tokens_.push_back(
            token);
      } else {
        // Error.
        break;
      }
    } else if (token_type == TOKEN_TYPE_STATEMENT_END) {
      if (!IsValidTokenPredecessor(last_token_type)) {
        // Error.
        break;
      }
    } else if (token_type == TOKEN_TYPE_START_BLOCK) {
      if (!IsValidTokenPredecessor(last_token_type)) {
        // Error.
        break;
      }
      NginxConfig* const new_config = new NginxConfig;
      config_stack.top()->statements_.back().get()->child_block_.reset(
          new_config);
      config_stack.push(new_config);
    } else if (token_type == TOKEN_TYPE_END_BLOCK) {
      if (last_token_type != TOKEN_TYPE_STATEMENT_END &&
          last_token_type != TOKEN_TYPE_END_BLOCK) { // fix nested config
        // Error.
        break;
      }
      // extra closing brace
      if (config_stack.size() == 1) {
        token_type = TOKEN_TYPE_ERROR;
        break;
      }
      config_stack.pop();
    } else if (token_type == TOKEN_TYPE_EOF) {
      if (last_token_type != TOKEN_TYPE_STATEMENT_END &&
          last_token_type != TOKEN_TYPE_END_BLOCK) { 
        // Error.
        break;
      }
      // unclosed open brace
      if (config_stack.size() != 1) {
        break;
      }
      return true;
    } else {
      // Error. Unknown token.
      break;
    }
    last_token_type = token_type;
  }

  printf ("Bad transition from %s to %s\n",
          TokenTypeAsString(last_token_type),
          TokenTypeAsString(token_type));
  BOOST_LOG_TRIVIAL(warning) << "Warning, bad transition from " << TokenTypeAsString(last_token_type) << " to " << TokenTypeAsString(token_type);
  return false;
}

bool NginxConfigParser::Parse(const char* file_name, NginxConfig* config) {
  std::ifstream config_file;
  config_file.open(file_name);
  if (!config_file.good()) {
    printf ("Failed to open config file: %s\n", file_name);
    BOOST_LOG_TRIVIAL(error) << "Error opening config file: " << file_name;
    return false;
  }

  const bool return_value =
      Parse(dynamic_cast<std::istream*>(&config_file), config);
  config_file.close();
  if (return_value == false) {
    BOOST_LOG_TRIVIAL(error) << "Config file parsing failed: " << file_name;
  } else {
    BOOST_LOG_TRIVIAL(info) << "Config file parsing succeeded: " << file_name;
  }
  return return_value;
}
