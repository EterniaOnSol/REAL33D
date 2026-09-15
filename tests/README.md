# Tests

No executable test suite exists yet. Protocol tests should cover both directions:

- known packet bytes -> expected typed semantic event;
- typed client command -> expected packet bytes.

Every parity test records preconditions, clients A/B, action, expected server/2D/3D results, actual result, evidence path, and one official status. Initial required IDs are `PARITY-LOGIN-001`, `PARITY-GAME-ENTRY-001`, bidirectional visibility, cardinal/blocked movement, and floor transitions.
