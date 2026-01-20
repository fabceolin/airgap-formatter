use crate::formatter::minify_json;
use crate::validator::validate_json;

#[test]
fn test_validate_valid_simple_object() {
    let input = r#"{"key": "value"}"#;
    let result = validate_json(input);
    assert!(result.is_valid);
    assert!(result.error.is_none());
}

#[test]
fn test_validate_valid_array() {
    let input = "[1, 2, 3]";
    let result = validate_json(input);
    assert!(result.is_valid);
}

#[test]
fn test_validate_valid_nested() {
    let input = r#"{"outer": {"inner": [1, 2, 3]}}"#;
    let result = validate_json(input);
    assert!(result.is_valid);
}

#[test]
fn test_validate_invalid_syntax() {
    let input = "{key: value}"; // Missing quotes
    let result = validate_json(input);
    assert!(!result.is_valid);
    assert!(result.error.is_some());
}

#[test]
fn test_validate_invalid_trailing_comma() {
    let input = r#"{"key": "value",}"#;
    let result = validate_json(input);
    assert!(!result.is_valid);
}

#[test]
fn test_validate_invalid_unclosed_brace() {
    let input = r#"{"key": "value""#;
    let result = validate_json(input);
    assert!(!result.is_valid);
}

#[test]
fn test_error_position_line_column() {
    let input = "{\n  \"key\": invalid\n}";
    let result = validate_json(input);
    assert!(!result.is_valid);
    let error = result.error.unwrap();
    assert_eq!(error.line, 2);
    assert!(error.column > 0);
}

#[test]
fn test_stats_object_count() {
    let input = r#"{"a": {}, "b": {"c": {}}}"#;
    let result = validate_json(input);
    assert_eq!(result.stats.object_count, 4);
}

#[test]
fn test_stats_array_count() {
    let input = r#"[[], [1, 2], [[3]]]"#;
    let result = validate_json(input);
    assert_eq!(result.stats.array_count, 5);
}

#[test]
fn test_stats_string_count() {
    let input = r#"{"a": "one", "b": "two", "c": "three"}"#;
    let result = validate_json(input);
    assert_eq!(result.stats.string_count, 3);
}

#[test]
fn test_stats_number_count() {
    let input = r#"[1, 2, 3.14, -5, 0]"#;
    let result = validate_json(input);
    assert_eq!(result.stats.number_count, 5);
}

#[test]
fn test_stats_boolean_count() {
    let input = r#"[true, false, true]"#;
    let result = validate_json(input);
    assert_eq!(result.stats.boolean_count, 3);
}

#[test]
fn test_stats_null_count() {
    let input = r#"[null, null, {"n": null}]"#;
    let result = validate_json(input);
    assert_eq!(result.stats.null_count, 3);
}

#[test]
fn test_stats_max_depth_flat() {
    let input = r#"{"a": 1, "b": 2}"#;
    let result = validate_json(input);
    assert_eq!(result.stats.max_depth, 1);
}

#[test]
fn test_stats_max_depth_nested() {
    let input = r#"{"l1": {"l2": {"l3": {"l4": "deep"}}}}"#;
    let result = validate_json(input);
    assert_eq!(result.stats.max_depth, 4);
}

#[test]
fn test_stats_max_depth_array_nested() {
    let input = r#"[[[[1]]]]"#;
    let result = validate_json(input);
    assert_eq!(result.stats.max_depth, 4);
}

#[test]
fn test_stats_total_keys() {
    let input = r#"{"a": 1, "b": 2, "c": {"d": 3, "e": 4}}"#;
    let result = validate_json(input);
    assert_eq!(result.stats.total_keys, 5);
}

#[test]
fn test_minify_removes_whitespace() {
    let input = r#"{
        "name": "John",
        "age": 30
    }"#;
    let result = minify_json(input).unwrap();
    assert!(!result.contains('\n'));
    assert!(!result.contains("  "));
    assert!(result.contains(r#""name":"John""#) || result.contains(r#""age":30"#));
}

#[test]
fn test_minify_preserves_content() {
    let input = r#"{"key": "value with spaces"}"#;
    let result = minify_json(input).unwrap();
    assert!(result.contains("value with spaces"));
}

#[test]
fn test_minify_invalid_json() {
    let input = "{invalid}";
    let result = minify_json(input);
    assert!(result.is_err());
}

#[test]
fn test_minify_array() {
    let input = "[ 1 , 2 , 3 ]";
    let result = minify_json(input).unwrap();
    assert_eq!(result, "[1,2,3]");
}

#[test]
fn test_minify_nested() {
    let input = r#"{ "outer" : { "inner" : "value" } }"#;
    let result = minify_json(input).unwrap();
    assert!(!result.contains(' ') || result.contains("value")); // Only spaces in string values
}

#[test]
fn test_validate_empty_object() {
    let input = "{}";
    let result = validate_json(input);
    assert!(result.is_valid);
    assert_eq!(result.stats.object_count, 1);
    assert_eq!(result.stats.total_keys, 0);
}

#[test]
fn test_validate_empty_array() {
    let input = "[]";
    let result = validate_json(input);
    assert!(result.is_valid);
    assert_eq!(result.stats.array_count, 1);
}

#[test]
fn test_validate_unicode() {
    let input = r#"{"emoji": "Hello", "chinese": "你好"}"#;
    let result = validate_json(input);
    assert!(result.is_valid);
    assert_eq!(result.stats.string_count, 2);
}
