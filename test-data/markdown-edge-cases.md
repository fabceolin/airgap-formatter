# Edge Cases Test Document

## Unicode and Special Characters

### Unicode Text

Привет мир (Russian)
你好世界 (Chinese)
مرحبا بالعالم (Arabic)
🎉 Emoji test 🚀 with multiple 💻 emojis 🔥

### Special Characters in Markdown

Text with \*escaped asterisks\* and \`escaped backticks\`.

Text with &amp; HTML entities &lt;tag&gt; and &quot;quotes&quot;.

## Deeply Nested Structures

> Level 1 quote
> > Level 2 quote
> > > Level 3 quote
> > > > Level 4 quote

- Level 1
  - Level 2
    - Level 3
      - Level 4
        - Level 5
          - Level 6

1. First
   1. First-First
      1. First-First-First
         1. First-First-First-First

## Empty and Whitespace Cases

Empty code block:
```
```

Code block with only whitespace:
```

```

Empty table cell:
| Header 1 | Header 2 |
|----------|----------|
|          | Content  |
| Content  |          |

## Long Lines

This is a very long line that should test how the renderer handles extremely long content without any line breaks or wrapping hints provided by the author of the document, which could potentially cause issues with horizontal scrolling or text overflow in the rendered output.

## Complex Tables

| Column 1 | Column 2 | Column 3 | Column 4 | Column 5 |
|----------|:---------|:--------:|---------:|----------|
| A | B | C | D | E |
| Very long content in this cell | Short | Center aligned | Right aligned | Normal |
| `code` | **bold** | *italic* | ~~strike~~ | [link](url) |

## Code Blocks Edge Cases

```
No language specified
```

```text
Plain text language
```

```unknownlanguage
Unknown language identifier
```

```javascript
// Very long line of code that exceeds typical display widths and should test horizontal scrolling behavior in code blocks
const longVariableName = "This is a very long string value that continues for quite some time";
```

## Inline Formatting Edge Cases

**Bold at** the **start and end**

*Italic at* the *start and end*

~~Strikethrough at~~ the ~~start and end~~

`Code at` the `start and end`

**Nested *italic* inside bold**

*Nested **bold** inside italic*

**Bold with `code` inside**

## List Edge Cases

- Item with **bold**
- Item with *italic*
- Item with `code`
- Item with [link](url)
- Item with
  continuation on next line

1. Numbered with **bold**
2. Numbered with *italic*
10. Number 10 (testing double digits)
100. Number 100 (testing triple digits)

## Horizontal Rules Variations

---

***

___

- - -

* * *

_ _ _

## Link Edge Cases

[Empty href]()

[Link with special chars](https://example.com/path?query=value&other=123#anchor)

[Link with parentheses](https://example.com/path_(with_parens))

## Image Edge Cases

![Alt with **markdown**](image.png)

![](empty-alt.png)

## Frontmatter (if supported)

---
title: Edge Cases Document
author: Test Author
date: 2024-01-01
tags:
  - test
  - edge-cases
---

This content appears after frontmatter.
