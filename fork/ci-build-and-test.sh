#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Builds dolphinCommander with its tests and checks the test run against
# fork/expected-test-failures.txt. Run from the top of the repository.
#
#   fork/ci-build-and-test.sh --install-deps   in a Manjaro or Arch Linux container (what the GitHub workflows do)
#   fork/ci-build-and-test.sh                  on a machine that has the build dependencies
#
# Writes the build to build-ci/ and the test summary to build-ci/test-summary.txt.
set -euo pipefail

if [[ "${1:-}" == "--install-deps" ]]; then
    # Dolphin's master wants recent KDE Frameworks, which Manjaro and Arch have and Ubuntu does not.
    pacman -Syu --noconfirm --needed \
        base-devel git cmake ninja python dbus xorg-server-xvfb appstream \
        extra-cmake-modules qt6-base qt6-multimedia qt6-tools \
        karchive kcmutils kcoreaddons ki18n kdbusaddons kbookmarks kconfig kio kparts solid \
        kiconthemes kcompletion ktextwidgets knotifications kcrash kwindowsystem \
        kwidgetsaddons kcodecs kguiaddons kcolorscheme kdoctools \
        packagekit-qt6 baloo baloo-widgets kfilemetadata breeze-icons \
        noto-fonts kio-extras
    # The tests measure names, so they need fonts, and they make thumbnails with kio-extras. They
    # also test read-only folders, which root can write to, so the rest runs as another user.
    id builder > /dev/null 2>&1 || useradd --create-home builder
    chown -R builder: .
    exec runuser -u builder -- "$0"
fi

cmake -S . -B build-ci -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
cmake --build build-ci

export QT_QPA_PLATFORM=offscreen
# The tests use KIO, which talks over D-Bus.
set +e
dbus-run-session -- ctest --test-dir build-ci -j"$(nproc)" --timeout 180 --output-junit "$PWD/build-ci/junit.xml" > build-ci/ctest.log 2>&1
first=$?
set -e
tail -n 30 build-ci/ctest.log

junit=build-ci/junit.xml
if [[ "$first" != 0 ]]; then
    # Some of Dolphin's tests depend on timing and fail now and then when they run side by side.
    # The suites that failed run again one by one, and only what fails then counts.
    set +e
    dbus-run-session -- ctest --test-dir build-ci --rerun-failed -j1 --timeout 180 --output-junit "$PWD/build-ci/junit-rerun.xml" > build-ci/ctest-rerun.log 2>&1
    set -e
    junit=build-ci/junit-rerun.xml
fi

python3 fork/check-tests.py "$junit" | tee build-ci/test-summary.txt
exit "${PIPESTATUS[0]}"
