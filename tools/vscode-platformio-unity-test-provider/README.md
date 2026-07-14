# PlatformIO Unity Test Provider

Workspace-local VS Code extension for the Haptic Console firmware repo.

It discovers `RUN_TEST(...)` calls in `test/**/test_main.cpp`, registers them
with VS Code's native Testing API, and runs:

```bash
pio test -e native_test
```

PlatformIO's native Unity flow executes the whole environment, so running a
single test from the Testing tab still invokes the full `native_test` run and
then maps Unity output lines back to discovered test items.
