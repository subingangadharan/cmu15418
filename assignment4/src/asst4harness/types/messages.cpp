// Copyright 2013 15418 Course Staff.

#include <map>
#include <string>
#include <sstream>

#include "server/messages.h"
#include "types/types.h"

/*
 * Trim --
 *
 * Returns the input string 'str' after removing leading and trailing
 * whitespace.
 */
std::string
Trim(const std::string& str) {
  size_t pos1 = str.find_first_not_of(" \t\n");
  size_t pos2 = str.find_last_not_of(" \t\n");

  if (pos1 == std::string::npos)
    return "";
  else if (pos2 == std::string::npos)
    return str.substr(pos1);
  else
    return str.substr(pos1, pos2-pos1+1);
}

/*
 * ParseKeyValue --
 *
 * Parse the source string into a key-value pair, where '=' seperates
 * the key from the value.  Returns true if parsing is successful.
 * Return false if the source string 'str' is not a proper key-value
 * pair
 */
bool
ParseKeyValue(std::string& key, std::string& value, const std::string& str) {
  size_t pos = str.find('=');

  // invalid format if there's no key, no value, or no '='
  if (pos == 0 ||
      pos + 1 == str.size() ||
      pos == std::string::npos) {
    key = "";
    value = "";
    return false;
  }

  key = Trim(str.substr(0, pos));
  value = Trim(str.substr(pos+1));
  return true;
}

class StringTokenizer {

  public:
  StringTokenizer(const std::string& str, const std::string& delimiters);
  void Reset();
  bool NoMoreTokens() const;
  std::string NextToken();

private:
  std::string sourceStr;
  std::string delimiters;
  size_t curTokenStart;
};

/*
 * StringTokenizer constructor --
 *
 * Initializes tokenizer with source string 'str'. Valid token
 * delimiters are characters in the 'delimit'.
 */
StringTokenizer::StringTokenizer(const std::string& str,
                                 const std::string& delimit) {
  sourceStr = str;
  delimiters = delimit;
  Reset();
}

/*
 * Reset --
 *
 * Resets the tokenizer.  Next call to NextToken() will return the
 * first token.
 */
void
StringTokenizer::Reset() {
  curTokenStart = sourceStr.find_first_not_of(delimiters, 0);
}

/*
 * NoMoreTokens --
 *
 * Returns true if there are no more tokens in the string.  False
 * otherwise.
 */
bool
StringTokenizer::NoMoreTokens() const {
  return (curTokenStart == std::string::npos);
}

/*
 * NextToken --
 *
 * Returns the next token in the string.  The current implementation
 * skips over empty tokens (adjacent delimiter characters in the
 * string will not cause NextToken to return an empty string as a
 * valid token).
 */
std::string
StringTokenizer::NextToken() {
  size_t tokenStart = curTokenStart;
  size_t tokenEnd = sourceStr.find_first_of(delimiters, curTokenStart);

  curTokenStart = sourceStr.find_first_not_of(delimiters, tokenEnd);

  if (tokenStart == std::string::npos)
    return "";
  else if (tokenEnd == std::string::npos)
    return sourceStr.substr(tokenStart);
  else
    return sourceStr.substr(tokenStart, tokenEnd - tokenStart);
}


Request_msg::Request_msg(int argTag) {
  tag = argTag;
}

Request_msg::Request_msg(int argTag, const std::string& str) {
  tag = argTag;
  StringTokenizer tok(str, ";");
  while (!tok.NoMoreTokens()) {
    std::string str = tok.NextToken();
    std::string key;
    std::string value;
    ParseKeyValue(key, value, str);
    if (key.size() != 0)
      dict[key] = value;
  }
}


Request_msg::Request_msg(int arg_tag, const Request_msg& r) {
  tag = arg_tag;
  dict = r.dict;
}

Request_msg::Request_msg(const Request_msg& r) {
  tag = r.tag;
  dict = r.dict;
}

void Request_msg::set_arg(const std::string& key, const std::string& value) {
  dict[key] = value;
}

std::string Request_msg::get_arg(const std::string& name) const {
  std::map<std::string, std::string>::const_iterator it = dict.find(name);
  if (it == dict.end())
    return "";
  else
    return it->second;
}

std::string Request_msg::get_request_string() const {

  // serialize dict

  bool first = true;
  std::ostringstream oss;
  for (std::map<std::string, std::string>::const_iterator it = dict.begin(); it != dict.end(); it++) {
    if (!first)
      oss << ";";
    oss << it->first << "=" << it->second;
    first = false;
  }

  return oss.str();
}
