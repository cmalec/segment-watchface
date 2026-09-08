---
description: |
  Write commit messages and PR titles in scoped-commit format ("scope: description"), not Conventional Commits. Use when writing commit messages, creating commits, titling pull requests, squash-merging, or when the user asks about commit message style. Scope is mandatory and comes first; type prefixes (feat/fix/chore) are dropped.
name: scoped-commit
---

# Scoped Commits

Scoped commits, not Conventional Commits. The scope is the most useful piece of information in a commit message, so it goes first and it is mandatory. The type (`feat`, `fix`, `chore`) is dropped: a good description already implies the type, and the type prefix wastes subject-line space on metadata nobody scans for.

Inspired by the commit conventions of Linux, Git, Go, and nixpkgs.

## When this skill applies

Always default to scoped commits. The only exception: the repo's `AGENTS.md`
or `CONTRIBUTING` files explicitly define a different commit style. In that case, follow the repo docs. Do not infer a convention from the git log — existing history is often inconsistent, and a popular style in the log is not an explicit project decision.

## Format

```
scope: description

[optional body]

[optional footer(s)]
```

- **Scope is required.** A commit without a scope is a sentence without a subject.
- **Description** is imperative, starts lowercase, no trailing period. It says what the change does, not what kind of change it is.
- **Subject line ≤ 72 characters.** Move detail to the body.
- **Body** (optional) explains *why*, links context, notes trade-offs. Wrap at 80.
- **PR number** is appended by squash-merge (`... (#123)`). Do not type it by hand.
- **No ticket IDs in the scope.** If a ticket reference is mandatory, put it in the body or a footer (`Refs: PROJ-1234`), never where the scope belongs.

## PR titles are commit messages

When the repo squash-merges via the GitHub UI, **the PR title becomes the commit subject line** on the default branch (GitHub appends the PR number). Therefore:

- PR titles follow the same `scope: description` format as commit messages.
- Write the PR title as the commit you want to land, not as a branch summary.
  `api: add measurement profile templates for coverage rules`, not `Measurement profiles feature branch`.
- The PR body maps to the commit body: put the *why* there. Trim GitHub template boilerplate before merging so it does not land in the commit log.
- In-branch commits can be looser (`wip`, `bump`) since squash-merge erases them — but keeping them scoped makes review easier and protects you if the PR ever merges without squashing.

## Choosing the scope

Use the **narrowest scope that honestly contains the change**:

1. **Subsystem** — the default. Top-level component of the repo.
2. **Package or area** — when the change is narrowly focused on one package, one provider, or one feature area, use that directly (optionally as
   `subsystem/area` for precision).

Derive scopes from the directory layout. When in doubt, the scope is the directory a reviewer would `cd` into. Check the repo for a documented scope catalog (for example a `COMMITS.md` or an `AGENTS.md` section) before inventing new scopes.

### Example catalogs

Scopes are repo-specific. Two illustrative catalogs:

**Monorepo (apps + libs + deploy):**

| Scope          | Covers                                          |
|----------------|-------------------------------------------------|
| `api`          | `apps/api` — control plane, HTTP handlers       |
| `api/<area>`   | focused areas: `auth`, `billing`, `webhooks`    |
| `web`          | `apps/web` — the frontend app                   |
| `worker`       | `apps/worker` — background jobs                 |
| `<libname>`    | `lib/go/*` packages — use the package name      |
| `deploy`       | `deploy/charts`, config, secrets bumps          |
| `docs`         | documentation-only changes                      |

**Single-app repo (like this one):** scope by source module or feature area — `timedigits`, `settings`, `pkjs`, `server`, `build` — whatever a reviewer would point at.

### Cross-cutting changes

- **Split the commit.** One logical change per commit. If a change touches two subsystems, it is usually two commits.
- If splitting is genuinely impossible (a rename across the repo, a schema change with its migration), use the **broadest common scope** and enumerate the affected areas in the body.

## What not to do

- `bump`, `bump chart`, `dummy change` — say *what* bumped and *why*.
- `fixing merge conflicts` — do not commit conflict-resolution noise; rebase instead, or if the resolution was substantive, describe the actual change.
- `cleanup` — cleanup of *what*? `api: drop unused provider_fixtures table` tells a story; `cleanup` does not.
- `addressing comment` — the comment is not visible in the log; the change is.
- Type-prefixed multi-change subjects like `feat: profiles page +
  rule switching; drop per-rule overrides` — split it and scope it.

## Rewrites from real history

| Before                                                                 | After                                                                 |
|------------------------------------------------------------------------|-----------------------------------------------------------------------|
| `feat(metrics): per-attempt callback timeouts, observability scope`    | `api/metrics: add per-attempt callback timeouts`                      |
| `fix(payments): use fixture amounts, drop legacy endpoint`             | `payments: source amounts from fixtures, drop legacy endpoint`        |
| `perf(worker): reduce redundant settlement observations`               | `worker: reduce redundant settlement observations`                    |
| `bumping up secrets`                                                   | `deploy: bump secrets to v5`                                          |
| `Mitigate fire-and-forget callback risks`                              | `worker: make callbacks at-least-once instead of fire-and-forget`     |
| `UI improvements`                                                      | `web/admin: <what actually improved>`                                 |
| `scheduling fix for sportsdata league api`                             | `sportsdata: fix league API scheduling`                               |
