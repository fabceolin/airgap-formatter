use crate::types::{FormatError, JsonStats, ValidationResult};
use serde_json::Value;

/// Validate JSON and return statistics about its structure.
///
/// # Arguments
/// * `input` - The JSON string to validate
///
/// # Returns
/// * `ValidationResult` containing validity status, error info (if invalid), and statistics
pub fn validate_json(input: &str) -> ValidationResult {
    match serde_json::from_str::<Value>(input) {
        Ok(value) => {
            let mut stats = JsonStats::default();
            collect_stats(&value, 0, &mut stats);
            ValidationResult::valid(stats)
        }
        Err(e) => {
            let error = FormatError::new(e.to_string(), e.line(), e.column());
            ValidationResult::invalid(error)
        }
    }
}

/// Recursively collect statistics from a JSON value tree.
fn collect_stats(value: &Value, depth: usize, stats: &mut JsonStats) {
    // Update max depth
    stats.max_depth = stats.max_depth.max(depth);

    match value {
        Value::Object(map) => {
            stats.object_count += 1;
            stats.total_keys += map.len();
            for v in map.values() {
                collect_stats(v, depth + 1, stats);
            }
        }
        Value::Array(arr) => {
            stats.array_count += 1;
            for v in arr {
                collect_stats(v, depth + 1, stats);
            }
        }
        Value::String(_) => stats.string_count += 1,
        Value::Number(_) => stats.number_count += 1,
        Value::Bool(_) => stats.boolean_count += 1,
        Value::Null => stats.null_count += 1,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_validate_valid_json() {
        let input = r#"{"name": "test"}"#;
        let result = validate_json(input);
        assert!(result.is_valid);
        assert!(result.error.is_none());
    }

    #[test]
    fn test_validate_invalid_json() {
        let input = "{invalid}";
        let result = validate_json(input);
        assert!(!result.is_valid);
        assert!(result.error.is_some());
    }

    #[test]
    fn test_stats_simple_object() {
        let input = r#"{"key": "value"}"#;
        let result = validate_json(input);
        assert_eq!(result.stats.object_count, 1);
        assert_eq!(result.stats.string_count, 1);
        assert_eq!(result.stats.total_keys, 1);
    }

    #[test]
    fn test_stats_all_types() {
        let input = r#"{
            "str": "text",
            "num": 42,
            "bool": true,
            "null": null,
            "arr": [1, 2],
            "obj": {}
        }"#;
        let result = validate_json(input);
        assert!(result.is_valid);
        assert_eq!(result.stats.object_count, 2);
        assert_eq!(result.stats.array_count, 1);
        assert_eq!(result.stats.string_count, 1);
        assert_eq!(result.stats.number_count, 3); // 42, 1, 2
        assert_eq!(result.stats.boolean_count, 1);
        assert_eq!(result.stats.null_count, 1);
    }

    #[test]
    fn test_stats_max_depth() {
        let input = r#"{"a": {"b": {"c": "deep"}}}"#;
        let result = validate_json(input);
        assert_eq!(result.stats.max_depth, 3);
    }

    #[test]
    fn test_stats_total_keys() {
        let input = r#"{"a": 1, "b": 2, "c": {"d": 3}}"#;
        let result = validate_json(input);
        assert_eq!(result.stats.total_keys, 4);
    }
}
