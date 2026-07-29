## ADDED Requirements

### Requirement: COMM Session-And-Downlink QoS Evidence Separates Command And File Verdicts

The verification evidence tree SHALL record command-policy proof and shared file/downlink proof for the COMM session-and-downlink QoS slice as separate, reviewable verdict surfaces.

#### Scenario: Command-policy evidence remains distinct

- **WHEN** the COMM session-and-downlink QoS change completes
- **THEN** reviewers SHALL be able to inspect separate evidence for S-band full-authority command admission, UHF backup low-risk admission, UHF backup high-risk rejection, and policy-driven session transition behavior

#### Scenario: File/downlink evidence remains distinct

- **WHEN** the COMM session-and-downlink QoS change completes
- **THEN** reviewers SHALL be able to inspect separate evidence for HK/DP arbitration behavior and bounded UHF-primary file/downlink behavior instead of inferring those from command-policy output alone
