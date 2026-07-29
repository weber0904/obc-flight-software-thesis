## ADDED Requirements

### Requirement: Installed Release Layout
The Raspberry Pi target SHALL separate immutable installed release payloads from mutable runtime state by using a fixed install root with distinct release and runtime areas, and the installed stack SHALL keep persistent data, staging data, and logs outside the versioned release payload so release switching does not overwrite mutable state.

#### Scenario: Install root preserves runtime data across releases
- **WHEN** a new Raspberry Pi bundle is installed under the governed install root
- **THEN** the release payload SHALL live under a versioned release directory while staging, persistent-data, and logs SHALL remain under shared runtime roots outside that release directory
