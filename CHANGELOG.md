# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),

## Unreleased

### Fixed
- A small memory leak and a thread-safety bug in `last_set()`
- Possible (but unlikely) crash on Linux if `PR_SET_MM` fails for some reason
- Potential hang when using command-line interface on systems where `ps` is a symlink
- Numerous typos in code and typos/grammar in documentation

## [1.1] - 2026-03-16

### Added
- Support for DragonFly BSD
- Ability to directly invoke the module and a wrapper script `processtitle`.
  These portably print out the list of process IDs and their titles in the 
  best possible way for each platform.

## [1.0] - 2026-03-13

### Added
- Official support for GraalPy

### Changed
- The project status is now Production/Stable

## [0.2] - 2026-03-09

### Fixed
- Output of plain `ps` (without `-ef` or `a` flags) on Linux
- Bogus cpp file removal messages on package uninstall

## [0.1.0] - 2026-03-08

### Added
- Initial version

[0.1.0]: https://github.com/gershnik/processtitle/releases/0.1.0
[0.2]: https://github.com/gershnik/processtitle/releases/0.2
[1.0]: https://github.com/gershnik/processtitle/releases/1.0
[1.1]: https://github.com/gershnik/processtitle/releases/1.1
