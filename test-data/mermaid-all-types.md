# Mermaid Diagram Types Test Suite

This document tests all supported Mermaid diagram types.

## 1. Flowchart (graph TD - Top Down)

```mermaid
graph TD
    A[Start] --> B{Is it working?}
    B -->|Yes| C[Great!]
    B -->|No| D[Debug]
    D --> B
    C --> E[End]
```

## 2. Flowchart (flowchart LR - Left to Right)

```mermaid
flowchart LR
    A[Input] --> B[Process]
    B --> C[Output]
    C --> D{Valid?}
    D -->|Yes| E[Done]
    D -->|No| A
```

## 3. Sequence Diagram

```mermaid
sequenceDiagram
    participant Alice
    participant Bob
    participant Charlie

    Alice->>Bob: Hello Bob!
    Bob-->>Alice: Hi Alice!
    Alice->>Charlie: Hello Charlie!
    Charlie-->>Alice: Hey!
    Bob->>Charlie: Hi there!
    Charlie-->>Bob: Hello!
```

## 4. Class Diagram

```mermaid
classDiagram
    Animal <|-- Duck
    Animal <|-- Fish
    Animal <|-- Zebra
    Animal: +int age
    Animal: +String gender
    Animal: +isMammal()
    Animal: +mate()
    class Duck{
        +String beakColor
        +swim()
        +quack()
    }
    class Fish{
        -int sizeInFeet
        -canEat()
    }
    class Zebra{
        +bool is_wild
        +run()
    }
```

## 5. State Diagram

```mermaid
stateDiagram-v2
    [*] --> Still
    Still --> [*]
    Still --> Moving
    Moving --> Still
    Moving --> Crash
    Crash --> [*]
```

## 6. Entity Relationship Diagram

```mermaid
erDiagram
    CUSTOMER ||--o{ ORDER : places
    ORDER ||--|{ LINE-ITEM : contains
    CUSTOMER {
        string name
        string email
    }
    ORDER {
        int orderNumber
        date created
    }
    LINE-ITEM {
        string product
        int quantity
        float price
    }
```

## 7. Gantt Chart

```mermaid
gantt
    title Project Timeline
    dateFormat  YYYY-MM-DD
    section Planning
    Requirements    :a1, 2024-01-01, 7d
    Design          :a2, after a1, 5d
    section Development
    Implementation  :a3, after a2, 14d
    Testing         :a4, after a3, 7d
    section Deployment
    Release         :a5, after a4, 3d
```

## 8. Pie Chart

```mermaid
pie title Distribution of Time
    "Development" : 40
    "Testing" : 25
    "Documentation" : 15
    "Meetings" : 10
    "Other" : 10
```

## 9. Mindmap

```mermaid
mindmap
    root((Main Topic))
        Branch 1
            Leaf 1.1
            Leaf 1.2
        Branch 2
            Leaf 2.1
            Leaf 2.2
            Leaf 2.3
        Branch 3
            Sub-branch 3.1
                Deep Leaf
```

## Multiple Diagrams in One Document

This section tests having multiple diagrams in sequence.

```mermaid
graph LR
    A --> B
```

Some text between diagrams.

```mermaid
pie title Simple Pie
    "A" : 50
    "B" : 50
```
