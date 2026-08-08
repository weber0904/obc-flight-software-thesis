# Tasks: payload-virtual-csp-node-v1

## 1. Governance

- [x] 1.1 Add proposal, design, tasks, and delta specs for payload virtual CSP node

## 2. Contract And Runtime

- [x] 2.1 Reserve payload virtual node `7` and service ports `40..49`
- [x] 2.2 Add payload CSP runtime types and gateway/service translation
- [x] 2.3 Wire the OBC topology to expose the payload service on node `1`
  payload-owned service ports while keeping node `7` reserved for future split
  deployment

## 3. Verification

- [x] 3.1 Add UT for translation and payload snapshot behavior
- [x] 3.2 Add hosted internal-CSP proof from a non-OBC client node into OBC
  node `1` payload service ports
