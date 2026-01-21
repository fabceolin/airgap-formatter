//! JSON syntax highlighter - Simple implementation without binary dependencies
//!
//! Provides syntax highlighting for JSON using a simple state machine parser.
//! Avoids syntect's binary serialization which has WASM compatibility issues.

/// Color palette (VS Code dark theme inspired)
mod colors {
    pub const STRING: &str = "#ce9178";      // Orange-ish for strings
    pub const KEY: &str = "#9cdcfe";         // Light blue for keys
    pub const NUMBER: &str = "#b5cea8";      // Light green for numbers
    pub const BOOLEAN: &str = "#569cd6";     // Blue for booleans
    pub const NULL: &str = "#569cd6";        // Blue for null
    pub const BRACKET: &str = "#ffd700";     // Gold for brackets
    pub const PUNCTUATION: &str = "#d4d4d4"; // Gray for colons, commas
}

/// Highlights JSON string and returns HTML with inline styles.
///
/// # Arguments
/// * `input` - The JSON string to highlight
///
/// # Returns
/// * HTML string with inline styles for syntax highlighting
/// * Empty string if input is empty
pub fn highlight_json(input: &str) -> String {
    if input.is_empty() {
        return String::new();
    }

    let mut output = String::with_capacity(input.len() * 3);
    output.push_str("<pre style=\"margin:0;font-family:inherit;\">");

    let chars: Vec<char> = input.chars().collect();
    let len = chars.len();
    let mut i = 0;

    // Track if we're expecting a key (after { or ,)
    let mut expect_key = false;
    let mut brace_stack: Vec<char> = Vec::new();

    while i < len {
        let c = chars[i];

        match c {
            // Whitespace - preserve as-is
            ' ' | '\t' | '\n' | '\r' => {
                output.push(c);
                i += 1;
            }

            // Object start
            '{' => {
                push_colored(&mut output, "{", colors::BRACKET);
                brace_stack.push('{');
                expect_key = true;
                i += 1;
            }

            // Object end
            '}' => {
                push_colored(&mut output, "}", colors::BRACKET);
                brace_stack.pop();
                expect_key = false;
                i += 1;
            }

            // Array start
            '[' => {
                push_colored(&mut output, "[", colors::BRACKET);
                brace_stack.push('[');
                expect_key = false;
                i += 1;
            }

            // Array end
            ']' => {
                push_colored(&mut output, "]", colors::BRACKET);
                brace_stack.pop();
                expect_key = false;
                i += 1;
            }

            // Colon (key-value separator)
            ':' => {
                push_colored(&mut output, ":", colors::PUNCTUATION);
                expect_key = false;
                i += 1;
            }

            // Comma
            ',' => {
                push_colored(&mut output, ",", colors::PUNCTUATION);
                // After comma in object, expect key; in array, expect value
                expect_key = brace_stack.last() == Some(&'{');
                i += 1;
            }

            // String (could be key or value)
            '"' => {
                let (string_content, end_pos) = parse_string(&chars, i);
                let color = if expect_key { colors::KEY } else { colors::STRING };
                push_colored(&mut output, &string_content, color);
                expect_key = false;
                i = end_pos;
            }

            // Number
            '-' | '0'..='9' => {
                let (num_str, end_pos) = parse_number(&chars, i);
                push_colored(&mut output, &num_str, colors::NUMBER);
                expect_key = false;
                i = end_pos;
            }

            // true
            't' if matches_keyword(&chars, i, "true") => {
                push_colored(&mut output, "true", colors::BOOLEAN);
                expect_key = false;
                i += 4;
            }

            // false
            'f' if matches_keyword(&chars, i, "false") => {
                push_colored(&mut output, "false", colors::BOOLEAN);
                expect_key = false;
                i += 5;
            }

            // null
            'n' if matches_keyword(&chars, i, "null") => {
                push_colored(&mut output, "null", colors::NULL);
                expect_key = false;
                i += 4;
            }

            // Unknown character - just escape and output
            _ => {
                push_escaped(&mut output, c);
                i += 1;
            }
        }
    }

    output.push_str("</pre>");
    output
}

/// Parse a JSON string starting at position i, returns (string_with_quotes, end_position)
fn parse_string(chars: &[char], start: usize) -> (String, usize) {
    let mut result = String::new();
    result.push('"');

    let mut i = start + 1; // Skip opening quote
    let len = chars.len();

    while i < len {
        let c = chars[i];
        match c {
            '"' => {
                result.push('"');
                return (result, i + 1);
            }
            '\\' if i + 1 < len => {
                result.push('\\');
                result.push(chars[i + 1]);
                i += 2;
            }
            '<' => {
                result.push_str("&lt;");
                i += 1;
            }
            '>' => {
                result.push_str("&gt;");
                i += 1;
            }
            '&' => {
                result.push_str("&amp;");
                i += 1;
            }
            _ => {
                result.push(c);
                i += 1;
            }
        }
    }

    // Unterminated string
    (result, i)
}

/// Parse a JSON number starting at position i, returns (number_string, end_position)
fn parse_number(chars: &[char], start: usize) -> (String, usize) {
    let mut result = String::new();
    let mut i = start;
    let len = chars.len();

    // Optional minus
    if i < len && chars[i] == '-' {
        result.push('-');
        i += 1;
    }

    // Integer part
    while i < len && chars[i].is_ascii_digit() {
        result.push(chars[i]);
        i += 1;
    }

    // Decimal part
    if i < len && chars[i] == '.' {
        result.push('.');
        i += 1;
        while i < len && chars[i].is_ascii_digit() {
            result.push(chars[i]);
            i += 1;
        }
    }

    // Exponent part
    if i < len && (chars[i] == 'e' || chars[i] == 'E') {
        result.push(chars[i]);
        i += 1;
        if i < len && (chars[i] == '+' || chars[i] == '-') {
            result.push(chars[i]);
            i += 1;
        }
        while i < len && chars[i].is_ascii_digit() {
            result.push(chars[i]);
            i += 1;
        }
    }

    (result, i)
}

/// Check if a keyword matches at position i
fn matches_keyword(chars: &[char], start: usize, keyword: &str) -> bool {
    let kw_chars: Vec<char> = keyword.chars().collect();
    if start + kw_chars.len() > chars.len() {
        return false;
    }
    for (j, kc) in kw_chars.iter().enumerate() {
        if chars[start + j] != *kc {
            return false;
        }
    }
    // Make sure keyword ends (not followed by alphanumeric)
    let end_pos = start + kw_chars.len();
    if end_pos < chars.len() && chars[end_pos].is_alphanumeric() {
        return false;
    }
    true
}

/// Push colored HTML span
fn push_colored(output: &mut String, text: &str, color: &str) {
    output.push_str("<span style=\"color:");
    output.push_str(color);
    output.push_str("\">");
    output.push_str(text);
    output.push_str("</span>");
}

/// Push escaped character
fn push_escaped(output: &mut String, c: char) {
    match c {
        '<' => output.push_str("&lt;"),
        '>' => output.push_str("&gt;"),
        '&' => output.push_str("&amp;"),
        _ => output.push(c),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_highlight_empty_input() {
        let result = highlight_json("");
        assert!(result.is_empty());
    }

    #[test]
    fn test_highlight_basic_json() {
        let input = r#"{"key": "value", "num": 42}"#;
        let result = highlight_json(input);
        // Should contain HTML spans for styling
        assert!(result.contains("<span"));
        assert!(result.contains("key"));
        assert!(result.contains("value"));
        assert!(result.contains("42"));
    }

    #[test]
    fn test_highlight_all_json_types() {
        let input = r#"{
  "string": "hello",
  "number": 123,
  "float": 3.14,
  "boolean_true": true,
  "boolean_false": false,
  "null_value": null,
  "array": [1, 2, 3],
  "object": {"nested": "value"}
}"#;
        let result = highlight_json(input);
        assert!(result.contains("<span"));
        assert!(result.contains("string"));
        assert!(result.contains("hello"));
        assert!(result.contains("123"));
        assert!(result.contains("true"));
        assert!(result.contains("false"));
        assert!(result.contains("null"));
    }

    #[test]
    fn test_highlight_key_vs_value_colors() {
        let input = r#"{"myKey": "myValue"}"#;
        let result = highlight_json(input);
        // Key should have KEY color
        assert!(result.contains(&format!("color:{}", colors::KEY)));
        // Value should have STRING color
        assert!(result.contains(&format!("color:{}", colors::STRING)));
    }

    #[test]
    fn test_highlight_escapes_html() {
        let input = r#"{"test": "<script>alert('xss')</script>"}"#;
        let result = highlight_json(input);
        assert!(result.contains("&lt;script&gt;"));
        assert!(!result.contains("<script>"));
    }
}
