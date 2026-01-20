use crate::formatter::format_json;
use crate::types::IndentStyle;
use crate::greet;
use std::time::Instant;

#[test]
fn test_greet() {
    assert_eq!(greet(), "Airgap JSON Formatter loaded successfully!");
}

#[test]
fn test_simple_object_2_spaces() {
    let input = r#"{"name":"John","age":30}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    let expected = r#"{
  "age": 30,
  "name": "John"
}"#;
    assert_eq!(result, expected);
}

#[test]
fn test_simple_object_4_spaces() {
    let input = r#"{"key":"value"}"#;
    let result = format_json(input, IndentStyle::Spaces(4)).unwrap();
    assert!(result.contains("    \"key\""));
}

#[test]
fn test_simple_object_tabs() {
    let input = r#"{"key":"value"}"#;
    let result = format_json(input, IndentStyle::Tabs).unwrap();
    assert!(result.contains("\t\"key\""));
}

#[test]
fn test_nested_objects() {
    let input = r#"{"level1":{"level2":{"level3":"deep"}}}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("\"level1\":"));
    assert!(result.contains("\"level2\":"));
    assert!(result.contains("\"level3\": \"deep\""));
}

#[test]
fn test_nested_arrays() {
    let input = r#"[[1,2],[3,4]]"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("[\n"));
    assert!(result.contains("1,"));
}

#[test]
fn test_mixed_nested() {
    let input = r#"{"arr":[1,2,{"nested":true}]}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("\"arr\": ["));
    assert!(result.contains("\"nested\": true"));
}

#[test]
fn test_empty_object() {
    let input = "{}";
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert_eq!(result, "{}");
}

#[test]
fn test_empty_array() {
    let input = "[]";
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert_eq!(result, "[]");
}

#[test]
fn test_unicode_characters() {
    let input = r#"{"emoji":"Hello, World!","chinese":"你好"}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("Hello, World!"));
    assert!(result.contains("你好"));
}

#[test]
fn test_special_characters_in_strings() {
    let input = r#"{"text":"line1\nline2\ttab"}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("\\n"));
    assert!(result.contains("\\t"));
}

#[test]
fn test_escaped_quotes() {
    let input = r#"{"quote":"He said \"hello\""}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("\\\"hello\\\""));
}

#[test]
fn test_all_value_types() {
    let input = r#"{"string":"text","number":42,"float":3.14,"bool":true,"null":null,"array":[1,2],"object":{}}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("\"text\""));
    assert!(result.contains("42"));
    assert!(result.contains("3.14"));
    assert!(result.contains("true"));
    assert!(result.contains("null"));
}

#[test]
fn test_invalid_json_error_position() {
    let input = "{\n  \"key\": invalid\n}";
    let result = format_json(input, IndentStyle::Spaces(2));
    assert!(result.is_err());
    let err = result.unwrap_err();
    assert_eq!(err.line, 2);
    assert!(err.column > 0);
}

#[test]
fn test_invalid_json_unclosed_brace() {
    let input = "{\"key\": \"value\"";
    let result = format_json(input, IndentStyle::Spaces(2));
    assert!(result.is_err());
}

#[test]
fn test_invalid_json_trailing_comma() {
    let input = r#"{"key": "value",}"#;
    let result = format_json(input, IndentStyle::Spaces(2));
    assert!(result.is_err());
}

#[test]
fn test_large_numbers() {
    let input = r#"{"big":9007199254740991,"negative":-123456789}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("9007199254740991"));
    assert!(result.contains("-123456789"));
}

#[test]
fn test_scientific_notation() {
    let input = r#"{"sci":1.23e10}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    // serde_json may convert notation
    assert!(result.contains("123") || result.contains("1.23e10") || result.contains("12300000000"));
}

#[test]
fn test_boolean_values() {
    let input = r#"{"yes":true,"no":false}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("true"));
    assert!(result.contains("false"));
}

#[test]
fn test_null_value() {
    let input = r#"{"nothing":null}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("null"));
}

#[test]
fn test_whitespace_preservation_in_strings() {
    let input = r#"{"spaces":"  leading and trailing  "}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("  leading and trailing  "));
}

#[test]
fn test_array_of_objects() {
    let input = r#"[{"id":1},{"id":2},{"id":3}]"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    assert!(result.contains("\"id\": 1"));
    assert!(result.contains("\"id\": 2"));
    assert!(result.contains("\"id\": 3"));
}

#[test]
fn test_deeply_nested_structure() {
    let input = r#"{"a":{"b":{"c":{"d":{"e":"deep"}}}}}"#;
    let result = format_json(input, IndentStyle::Spaces(2)).unwrap();
    // Count indentation levels
    let lines: Vec<&str> = result.lines().collect();
    assert!(lines.len() > 5);
}

#[test]
#[ignore] // Run with: cargo test --release -- --ignored
fn test_performance_1mb_json() {
    // Generate approximately 1MB of JSON
    let mut items = Vec::new();
    for i in 0..10000 {
        items.push(format!(
            r#"{{"id":{},"name":"Item {}","description":"This is a longer description for item number {} to add more content","value":{},"active":{}}}"#,
            i, i, i, i as f64 * 1.5, i % 2 == 0
        ));
    }
    let input = format!("[{}]", items.join(","));

    // Verify size is approximately 1MB
    let size_kb = input.len() / 1024;
    assert!(size_kb >= 900, "Generated JSON should be at least 900KB, got {}KB", size_kb);

    // Time the format operation
    let start = Instant::now();
    let result = format_json(&input, IndentStyle::Spaces(2));
    let duration = start.elapsed();

    // Verify it succeeds
    assert!(result.is_ok(), "Format should succeed");

    // Verify performance requirement: <100ms (only valid in release mode)
    let duration_ms = duration.as_millis();
    assert!(
        duration_ms < 100,
        "Format should complete in <100ms, took {}ms",
        duration_ms
    );

    println!("Performance test: {}KB formatted in {}ms", size_kb, duration_ms);
}
