# Contributing to CTF MPS KTX

English | [Português (Brasil)](CONTRIBUTING.pt-BR.md)

The creator and maintainer of this port is **Fernando (Droni) Salvatori**, [@fernandosalvatori](https://github.com/fernandosalvatori). The project was first published on **28 September 2026**, preserving the existing authorship of KTX, MVDSV, ServerModules and the other components.

## Review and approval

**Every pull request from another contributor requires review and approval by @fernandosalvatori before it can be merged into this repository. The required final approval belongs exclusively to the maintainer.** Reviews by other people and automated tests can support the assessment, but do not replace this approval.

GitHub does not allow authors to approve their own PRs. The maintainer can merge their own PRs through an administrative bypass, always through a PR. This bypass also allows the owner to merge other PRs at their discretion; it does not record a review approval that did not occur. No other collaborators had write access at the initial publication.

Opening a PR, passing tests or receiving favorable discussion does not constitute approval. After substantive changes to the diff, provide updated results and wait for review of the final version. The maintainer decides whether a change belongs in the project and which version will include it.

This policy governs contributions merged into this repository. It does not change the code license or imply endorsement by the upstream projects.

## Preparing a change

Keep each PR focused on an identifiable problem or behavior change. Preserve license and authorship headers. Before changing a weapon formula or rule, consult [docs/CHANGES.md](docs/CHANGES.md) and identify whether that behavior comes from KTX, ServerModules or a deliberate adaptation in this port.

When a change affects several layers, explain their relationship: for example, weapon selection, ammunition use, projectile creation, damage, death and respawn cleanup. Avoid mixing server configuration preferences with code fixes that can be reviewed independently.

## What to include in the PR description

- **Problem or objective:** the concrete situation that motivated the change and the expected outcome.
- **Before and after:** observable behavior, with an example that reviewers can reproduce.
- **Explanation of the diff:** relevant files and functions, the reasons for changing them and possible effects on other modes.
- **Origin of the rules:** reference parameters or routines, with attribution and a clear account of deliberate differences.
- **Validation:** commands run, compiler and engine versions, results and regression cases covered.
- **Limitations:** untested scenarios, known differences and anything still requiring a human playtest or visual verification.

A clear description must support review without access to private conversations. Do not present compilation, assertion counts or map loading as proof of complete gameplay validation.

## Build and tests

The documented workflow uses Python 3.9 or later and Zig 0.13.0 on Windows x64. From the repository root:

```text
python scripts/build.py --zig zig
python scripts/test.py --zig zig
```

The scripts accept an executable path through `--zig`; see [docs/BUILD.md](docs/BUILD.md) for `ZIG_EXE`, outputs and options. Record the outcome of the current run instead of copying a historical count without running the tests.

For changes to damage, selection or entity lifecycle, add cases that reproduce the problem and exercise the fix. Pay attention to respawn, disconnection, slot reuse, teammate and self-damage, death callbacks and entity removal. Projectile or hook changes also need engine validation when they affect collision or movement.

`tests/runtime.c` is a separate integration fixture, not part of the normal DLL produced by the build command above. If you use instrumentation, identify it in the results and keep it out of the production module.

Do not claim visual testing, human gameplay or a remote CI run that did not happen. If a validation step cannot be completed, state the limitation explicitly.

## Content to exclude from PRs

- PAKs, BSP/ENT maps, models, sounds or other assets from local installations or the commercial game.
- Engine executables, compiled binaries, caches or generated artifacts that are not part of the reviewable source.
- Credentials, private configuration, tokens, personal service addresses or player data.
- Copies of personal installations, full logs or diagnostic files whose contents have not been reviewed.

External resources required for engine tests must be provided locally. Include only the code, steps and evidence needed for review in the PR, removing private data from log excerpts. The local importer does not grant a license to redistribute imported files.

## Authorship and license

Preserve **Johannes Plass**'s attribution in the ServerModules modules, along with the other authors' notices in KTX components and earlier code. Identify the authorship and origin of new or adapted code without attributing this port's changes to upstream authors.

Read [LICENSE.md](LICENSE.md) and [LICENSES/NOTICE.md](LICENSES/NOTICE.md). If a contribution depends on code or content from another source, identify that source and its terms so compatibility can be assessed during review.
