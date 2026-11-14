# Keeping dolphinCommander up to date with Dolphin

dolphinCommander is a fork of KDE's Dolphin (https://invent.kde.org/system/dolphin, mirrored at
https://github.com/KDE/dolphin). Its own work lives on the `dolphincommander` branch, and Dolphin's
`master` is merged into that branch from time to time. Merging, not rebasing, keeps the fork's
history as it is and resolves each conflict once.

## What runs on its own

`.github/workflows/upstream-sync.yml` runs every Monday (and from the Actions tab, "Run workflow"):

| What happened | What you get |
| --- | --- |
| Nothing new in Dolphin | nothing |
| Merged, built, no new test failures | a pull request to review and merge |
| Merged, but a test that should pass fails, or the build fails | a draft pull request with the failures |
| Conflicts that need a person | an issue with the files, no pull request |

It builds in a Manjaro (stable branch) container: Ubuntu's KDE Frameworks are too old for Dolphin's
`master`, and what is built against Manjaro stable runs on Arch Linux too, which is not true the other
way round.

For the workflow to open pull requests, the repository needs Settings → Actions → General →
"Allow GitHub Actions to create and approve pull requests".

## Releases

`.github/workflows/release.yml` runs every Tuesday (and from the Actions tab). When the
`dolphincommander` branch has commits that the last release does not have, it builds and tests
them, tags `v<year>.<month>.<day>`, builds the Arch package from that tag with
`fork/arch/PKGBUILD`, and makes a GitHub release with the changes and the package. If the tests
fail, there is no release.

The package installs under `/opt/dolphincommander`, so that it can be installed next to Dolphin:
both have `libdolphinprivate`, the same settings files and the same translations. `/usr/bin/dolphincommander`
is a launcher that shows the program its plugins and translations there.

### The AUR

The release workflow also updates the `dolphincommander` package in the AUR, once it can push
there. That takes, once:

1. An account on https://aur.archlinux.org.
2. An SSH key for the workflow only: `ssh-keygen -t ed25519 -f aur_dolphincommander -N "" -C "dolphincommander release workflow"`.
   Add the `.pub` file to the account (My Account → SSH Public Key).
3. The private key as the secret `AUR_SSH_PRIVATE_KEY` of the GitHub repository
   (Settings → Secrets and variables → Actions), then delete the local copy.

The package does not have to exist in the AUR first: the first release that has the key pushes it
there, which creates it.

## Merging by hand (or when the workflow stopped at conflicts)

```bash
fork/merge-upstream.sh
```

It makes a branch `upstream-sync/<date>` from the current branch and merges Dolphin's `master`.
Two kinds of conflict always go the fork's way and are resolved by the script:

- a file that the fork deleted stays deleted (Dolphin's handbook images and translated handbooks,
  `org.kde.dolphin.desktop`, the Folders panel, ...);
- the handbook in `doc/` is the fork's own.

Every other conflict is left in the files. When they are resolved:

```bash
git add -A && git commit --no-edit
fork/ci-build-and-test.sh   # builds into build-ci/ and checks the tests, as the workflow does
```

`rerere` is turned on by the script, so a conflict that was resolved once is resolved the same way
when it comes again.

### How to resolve a conflict

- **Keep what the fork does, take what Dolphin adds.** A conflict is mostly the two sides changing
  the same few lines for different reasons: the result needs both.
- **When Dolphin moves code, the fork's lines move with it.** In the merge of 2026-11, Dolphin moved
  the connections of a view into `DolphinViewContainer::connectViewSignals()` and
  `DolphinView::connectItemController()`, so that they also apply to a view that replaces another.
  The fork's type-to-filter and Escape lines had to go there too, or they would have been lost
  without a conflict to show it.
- **Look at what merged without a conflict too.** Code that merges cleanly can still be wrong
  together, like Dolphin's new pinned tabs next to the fork's dragging of tabs between the sides.
- **A test of Dolphin that fails because of what the fork changed** goes in
  `expected-test-failures.txt`, with the reason. Do not change Dolphin's tests to pass on the fork
  unless they do not build: every change to them is a conflict at the next merge.

## Tests

- Dolphin's own tests, plus `src/tests/dolphincommanderfeaturestest.cpp`, which tests the fork's
  features (the two sides and moving tabs between them, the header, the Miller columns, typing to
  filter, details as the default). A new feature of the fork gets a test there, so a merge that
  breaks it fails.
- `check-tests.py` compares a run with `expected-test-failures.txt` and fails only for a failure
  that is not listed there. It also lists the expected ones that passed, which are flaky or fixed.

## Writing fork code that merges well

- Put new things in new files (`sidessplitter.cpp`, `views/columnsstrip.cpp`, `views/viewheader.cpp`).
- Where Dolphin's code has to change, keep the change small and in one place, and keep the fork's
  part in its own function when it can be (`DolphinViewContainer::connectForkViewSignals()`).
- Do not reformat or reorder Dolphin's code: it makes conflicts out of nothing.
