use std::fmt;

/// Indentation style for JSON formatting.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum IndentStyle {
    /// Use spaces for indentation (typically 2 or 4)
    Spaces(u8),
    /// Use tabs for indentation
    Tabs,
}

impl Default for IndentStyle {
    fn default() -> Self {
        IndentStyle::Spaces(4)
    }
}

impl IndentStyle {
    /// Get the string representation for one level of indentation.
    pub fn as_str(&self) -> String {
        match self {
            IndentStyle::Spaces(n) => " ".repeat(*n as usize),
            IndentStyle::Tabs => "\t".to_string(),
        }
    }
}

/// Error that occurs during JSON formatting or parsing.
#[derive(Clone, Debug, PartialEq)]
pub struct FormatError {
    pub message: String,
    pub line: usize,
    pub column: usize,
}

impl FormatError {
    pub fn new(message: impl Into<String>, line: usize, column: usize) -> Self {
        Self {
            message: message.into(),
            line,
            column,
        }
    }
}

impl fmt::Display for FormatError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "Error at line {}, column {}: {}",
            self.line, self.column, self.message
        )
    }
}

impl std::error::Error for FormatError {}

/// Statistics about a JSON document's structure.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct JsonStats {
    pub object_count: usize,
    pub array_count: usize,
    pub string_count: usize,
    pub number_count: usize,
    pub boolean_count: usize,
    pub null_count: usize,
    pub max_depth: usize,
    pub total_keys: usize,
}

/// Result of validating a JSON document.
#[derive(Clone, Debug)]
pub struct ValidationResult {
    pub is_valid: bool,
    pub error: Option<FormatError>,
    pub stats: JsonStats,
}

impl ValidationResult {
    /// Create a validation result for valid JSON.
    pub fn valid(stats: JsonStats) -> Self {
        Self {
            is_valid: true,
            error: None,
            stats,
        }
    }

    /// Create a validation result for invalid JSON.
    pub fn invalid(error: FormatError) -> Self {
        Self {
            is_valid: false,
            error: Some(error),
            stats: JsonStats::default(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_indent_style_default() {
        assert_eq!(IndentStyle::default(), IndentStyle::Spaces(4));
    }

    #[test]
    fn test_indent_style_as_str() {
        assert_eq!(IndentStyle::Spaces(2).as_str(), "  ");
        assert_eq!(IndentStyle::Spaces(4).as_str(), "    ");
        assert_eq!(IndentStyle::Tabs.as_str(), "\t");
    }

    #[test]
    fn test_format_error_display() {
        let err = FormatError::new("unexpected token", 5, 10);
        assert_eq!(err.to_string(), "Error at line 5, column 10: unexpected token");
    }

    #[test]
    fn test_format_error_new() {
        let err = FormatError::new("test error", 1, 2);
        assert_eq!(err.message, "test error");
        assert_eq!(err.line, 1);
        assert_eq!(err.column, 2);
    }

    #[test]
    fn test_json_stats_default() {
        let stats = JsonStats::default();
        assert_eq!(stats.object_count, 0);
        assert_eq!(stats.array_count, 0);
        assert_eq!(stats.max_depth, 0);
    }

    #[test]
    fn test_validation_result_valid() {
        let stats = JsonStats {
            object_count: 1,
            array_count: 2,
            ..Default::default()
        };
        let result = ValidationResult::valid(stats.clone());
        assert!(result.is_valid);
        assert!(result.error.is_none());
        assert_eq!(result.stats.object_count, 1);
    }

    #[test]
    fn test_validation_result_invalid() {
        let err = FormatError::new("syntax error", 1, 5);
        let result = ValidationResult::invalid(err);
        assert!(!result.is_valid);
        assert!(result.error.is_some());
        assert_eq!(result.error.unwrap().message, "syntax error");
    }
}
