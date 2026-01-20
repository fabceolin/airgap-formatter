use crate::types::{FormatError, IndentStyle};
use serde_json::Value;

/// Minify JSON by removing all unnecessary whitespace.
///
/// # Arguments
/// * `input` - The JSON string to minify
///
/// # Returns
/// * `Ok(String)` - The minified JSON string
/// * `Err(FormatError)` - Error with line/column position if JSON is invalid
pub fn minify_json(input: &str) -> Result<String, FormatError> {
    let value: Value = serde_json::from_str(input).map_err(|e| {
        FormatError::new(e.to_string(), e.line(), e.column())
    })?;

    // serde_json::to_string produces compact JSON without whitespace
    serde_json::to_string(&value).map_err(|e| {
        FormatError::new(e.to_string(), 0, 0)
    })
}

/// Format JSON with the specified indentation style.
///
/// # Arguments
/// * `input` - The JSON string to format
/// * `indent` - The indentation style to use
///
/// # Returns
/// * `Ok(String)` - The formatted JSON string
/// * `Err(FormatError)` - Error with line/column position if JSON is invalid
pub fn format_json(input: &str, indent: IndentStyle) -> Result<String, FormatError> {
    let value: Value = serde_json::from_str(input).map_err(|e| {
        FormatError::new(
            e.to_string(),
            e.line(),
            e.column(),
        )
    })?;

    let indent_str = indent.as_str();
    let mut output = String::with_capacity(input.len() * 2);
    format_value(&value, &indent_str, 0, &mut output);
    Ok(output)
}

/// Recursively format a JSON value with proper indentation.
fn format_value(value: &Value, indent_str: &str, depth: usize, output: &mut String) {
    match value {
        Value::Null => output.push_str("null"),
        Value::Bool(b) => output.push_str(if *b { "true" } else { "false" }),
        Value::Number(n) => output.push_str(&n.to_string()),
        Value::String(s) => {
            output.push('"');
            for c in s.chars() {
                match c {
                    '"' => output.push_str("\\\""),
                    '\\' => output.push_str("\\\\"),
                    '\n' => output.push_str("\\n"),
                    '\r' => output.push_str("\\r"),
                    '\t' => output.push_str("\\t"),
                    c if c.is_control() => {
                        output.push_str(&format!("\\u{:04x}", c as u32));
                    }
                    c => output.push(c),
                }
            }
            output.push('"');
        }
        Value::Array(arr) => {
            if arr.is_empty() {
                output.push_str("[]");
            } else {
                output.push_str("[\n");
                for (i, item) in arr.iter().enumerate() {
                    push_indent(output, indent_str, depth + 1);
                    format_value(item, indent_str, depth + 1, output);
                    if i < arr.len() - 1 {
                        output.push(',');
                    }
                    output.push('\n');
                }
                push_indent(output, indent_str, depth);
                output.push(']');
            }
        }
        Value::Object(obj) => {
            if obj.is_empty() {
                output.push_str("{}");
            } else {
                output.push_str("{\n");
                let len = obj.len();
                for (i, (key, val)) in obj.iter().enumerate() {
                    push_indent(output, indent_str, depth + 1);
                    output.push('"');
                    output.push_str(key);
                    output.push_str("\": ");
                    format_value(val, indent_str, depth + 1, output);
                    if i < len - 1 {
                        output.push(',');
                    }
                    output.push('\n');
                }
                push_indent(output, indent_str, depth);
                output.push('}');
            }
        }
    }
}

/// Push indentation to the output string.
fn push_indent(output: &mut String, indent_str: &str, depth: usize) {
    for _ in 0..depth {
        output.push_str(indent_str);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_format_simple_object() {
        let input = r#"{"name":"test","value":42}"#;
        let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
        assert!(result.contains("\"name\": \"test\""));
        assert!(result.contains("\"value\": 42"));
    }

    #[test]
    fn test_format_empty_object() {
        let input = "{}";
        let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
        assert_eq!(result, "{}");
    }

    #[test]
    fn test_format_empty_array() {
        let input = "[]";
        let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
        assert_eq!(result, "[]");
    }

    #[test]
    fn test_format_nested() {
        let input = r#"{"outer":{"inner":"value"}}"#;
        let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
        assert!(result.contains("{\n"));
        assert!(result.contains("  \"outer\""));
    }

    #[test]
    fn test_format_invalid_json() {
        let input = "{invalid}";
        let result = format_json(input, IndentStyle::Spaces(2));
        assert!(result.is_err());
        let err = result.unwrap_err();
        assert!(err.line > 0);
        assert!(err.column > 0);
    }

    #[test]
    fn test_format_with_tabs() {
        let input = r#"{"key":"value"}"#;
        let result = format_json(input, IndentStyle::Tabs).unwrap();
        assert!(result.contains("\t\"key\""));
    }

    #[test]
    fn test_minify_json() {
        let input = r#"{
            "name": "test",
            "value": 42
        }"#;
        let result = minify_json(input).unwrap();
        assert!(!result.contains('\n'));
        assert!(!result.contains("  "));
    }

    #[test]
    fn test_minify_invalid_json() {
        let input = "{invalid}";
        let result = minify_json(input);
        assert!(result.is_err());
    }
}
