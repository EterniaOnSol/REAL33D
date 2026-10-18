# Independent reproduction

Candidate 63a62ddf9477cb8ccdc91525d7f505fee192f2f4; base370b5676.
Use a fresh isolated worktree. Do not reuse an existing automation report.

1. Build the ClientCore MSVC library with tests/build_clientcore_windows.cmd
   in a Visual Studio x64 developer environment. Require all thirteen suites.
2. Configure clientcore with CMake/GCC and run ctest --output-on-failure;
   require thirteen named tests, not just exit0. Compile the four native tests
   with -std=c++17 -Iunreal/REAL33D/Source/REAL33D/Public. Supply the existing
   runtime game/reference/dat/objects.srv to real33d_item_use_policy_tests.
3. Copy independent-skills-probe.cpp into Source/REAL33D/Private temporarily.
   Build REAL33DEditor Win64 Development with Unreal5.8 Build.bat, -WaitMutex
   -NoUBA -MaxParallelActions=1 and the isolated uproject's absolute path.
4. Run scripts/client/test_unreal_presentation.ps1 with a new evidence directory
   and visual/qa/full_catalog_v08/experimental_catalog_runtime.json. Independently
   require all six named JSON entries Success, errors0, failed/notRun/inProcess0.
   The extra CertificationSkillsWidgets test paints the real bars and inspects
   actual widgets, including independent known flags and received zero.
5. Materialize Git LFS UI resources and extract classic item sprites using the
   project's existing extractor. Reuse available V08/Brother asset directories
   through junctions; never edit the shared targets. Run the normal client with
   presentation QA and the opt-in -real33d-cert-skills-observer. It only observes
   real event copies and actual Slate values after frames. No test fixture is
   installed in WorldState. Obtain before/after F9 and legitimate live skill
   progression. Check SkillsCert LIVE entries against displayed rows and bars.
6. Close the client normally, preserve its live log/JSON/screenshots and check
   the final EndPlay and every snapshot for residual/unsupported/anomalies0.
7. Remove the diagnostic from production Source. Build the exact candidate
   sources again. If UBT retains the removed file, remove only this worktree's
   regenerable SourceFileCache.bin/Makefile.bin after checking absolute paths.
8. Run tests/regression_harness_selftest.sh; require10/10. Run corrected harness
   with --unreal --strict --objects-srv and --logs pointing to this live session.
   Require0FAIL and complete ClientCore13 in its raw gate log.
9. Run tests/secret_check.sh directly under Windows Git Bash, with this checkout
   as cwd; require6 ok lines. WSL git cannot resolve Windows linked gitdir paths,
   so WSL secret_check is not valid evidence.

The initial native command in native-clientcore.txt omitted the public include
path. Its thirteen ClientCore tests passed before that setup failure. The corrected
native-only.txt is the actual four-suite pass. No candidate source was changed.