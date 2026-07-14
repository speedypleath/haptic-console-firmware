const vscode = require("vscode");
const cp = require("child_process");
const fs = require("fs");
const os = require("os");
const path = require("path");

const RUN_TEST_RE = /\bRUN_TEST\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*;/g;
const UNITY_RESULT_RE = /^(.*?):(\d+):([^:\s]+):(PASS|FAIL|IGNORE)(?::(.*))?$/;
const PLATFORMIO_RESULT_RE = /^(.*?):(\d+):\s*([^:\s]+)\s+\[(PASSED|FAILED|IGNORED)\](?::\s*(.*))?$/;
const ANSI_RE = /\x1b\[[0-9;]*m/g;

/** @type {vscode.TestController | undefined} */
let controller;

/** @type {Map<string, vscode.TestItem>} */
const testItemsByName = new Map();

function workspaceRoot() {
  const folders = vscode.workspace.workspaceFolders;
  return folders && folders.length > 0 ? folders[0].uri.fsPath : undefined;
}

function config() {
  const cfg = vscode.workspace.getConfiguration("platformioUnityTests");
  return {
    command: cfg.get("command", "pio"),
    environment: cfg.get("environment", "native_test"),
  };
}

async function findTestFiles() {
  return vscode.workspace.findFiles("test/**/test_main.cpp", "{.pio,build,**/.git}/**");
}

async function readText(uri) {
  const bytes = await vscode.workspace.fs.readFile(uri);
  return Buffer.from(bytes).toString("utf8");
}

function lineColumnForOffset(text, offset) {
  const prefix = text.slice(0, offset);
  const lines = prefix.split(/\r?\n/);
  return {
    line: lines.length - 1,
    character: lines[lines.length - 1].length,
  };
}

function suiteIdForUri(uri) {
  const root = workspaceRoot();
  return root ? path.relative(root, uri.fsPath) : uri.fsPath;
}

async function refreshTests() {
  if (!controller) return;

  testItemsByName.clear();
  controller.items.replace([]);

  const files = await findTestFiles();
  for (const uri of files) {
    const text = await readText(uri);
    const suiteId = suiteIdForUri(uri);
    const suite = controller.createTestItem(suiteId, suiteId, uri);
    suite.canResolveChildren = false;
    controller.items.add(suite);

    RUN_TEST_RE.lastIndex = 0;
    for (let match; (match = RUN_TEST_RE.exec(text)); ) {
      const name = match[1];
      const id = `${suiteId}::${name}`;
      const item = controller.createTestItem(id, name, uri);
      const pos = lineColumnForOffset(text, match.index);
      item.range = new vscode.Range(pos.line, pos.character, pos.line, pos.character + match[0].length);
      suite.children.add(item);
      testItemsByName.set(name, item);
    }
  }
}

function collectRequestedTests(request) {
  if (!request.include || request.include.length === 0) {
    return Array.from(testItemsByName.values());
  }

  const result = [];
  const visit = (item) => {
    if (item.children.size === 0) {
      result.push(item);
      return;
    }
    item.children.forEach(visit);
  };
  request.include.forEach(visit);
  return result;
}

function parseUnityResults(output) {
  const results = new Map();
  for (const line of output.split(/\r?\n/)) {
    const cleanLine = line.replace(ANSI_RE, "").trim();
    const unityMatch = UNITY_RESULT_RE.exec(cleanLine);
    const pioMatch = PLATFORMIO_RESULT_RE.exec(cleanLine);
    if (!unityMatch && !pioMatch) continue;

    const match = unityMatch || pioMatch;
    const [, file, lineNo, testName, rawStatus, message] = match;
    const status = rawStatus === "PASSED"
      ? "PASS"
      : rawStatus === "FAILED"
        ? "FAIL"
        : rawStatus === "IGNORED"
          ? "IGNORE"
          : rawStatus;
    results.set(testName, {
      file,
      line: Number(lineNo),
      status,
      message: message || "",
    });
  }
  return results;
}

function platformioTmpDir() {
  return path.join(os.homedir(), ".platformio", ".cache", "tmp");
}

async function readPlatformioJsonResults(runStartedAtMs) {
  const root = workspaceRoot();
  if (!root) return new Map();

  const { environment } = config();
  const tmpDir = platformioTmpDir();
  const results = new Map();

  let entries;
  try {
    entries = await fs.promises.readdir(tmpDir, { withFileTypes: true });
  } catch {
    return results;
  }

  const jsonFiles = [];
  for (const entry of entries) {
    if (!entry.isFile() || !/^test-list-\d+\.json$/.test(entry.name)) continue;
    const file = path.join(tmpDir, entry.name);
    try {
      const stat = await fs.promises.stat(file);
      // Allow a small clock skew window. These files are created by the pio
      // process we just launched and contain the authoritative PlatformIO
      // status model, including ERRORED cases.
      if (stat.mtimeMs >= runStartedAtMs - 2000) {
        jsonFiles.push({ file, mtimeMs: stat.mtimeMs });
      }
    } catch {
      // Ignore files removed by PlatformIO while scanning.
    }
  }

  jsonFiles.sort((a, b) => a.mtimeMs - b.mtimeMs);
  for (const { file } of jsonFiles) {
    let report;
    try {
      report = JSON.parse(await fs.promises.readFile(file, "utf8"));
    } catch {
      continue;
    }

    if (report.project_dir !== root || !Array.isArray(report.test_suites)) {
      continue;
    }

    for (const suite of report.test_suites) {
      if (suite.env_name !== environment || !Array.isArray(suite.test_cases)) {
        continue;
      }

      for (const testCase of suite.test_cases) {
        if (!testCase || !testCase.name) continue;
        const source = testCase.source || {};
        results.set(testCase.name, {
          file: source.file || undefined,
          line: typeof source.line === "number" ? source.line : undefined,
          status: testCase.status || suite.status,
          message: testCase.message || testCase.exception || testCase.stdout || "",
          duration: typeof testCase.duration === "number" ? testCase.duration * 1000 : 0,
        });
      }
    }
  }

  return results;
}

function runPioTest(token) {
  const root = workspaceRoot();
  if (!root) {
    return Promise.reject(new Error("No workspace folder is open."));
  }

  const { command, environment } = config();
  return new Promise((resolve, reject) => {
    const child = cp.spawn(command, ["test", "-e", environment], {
      cwd: root,
      shell: process.platform === "win32",
    });

    let output = "";
    const append = (chunk) => {
      output += chunk.toString();
    };

    child.stdout.on("data", append);
    child.stderr.on("data", append);
    child.on("error", reject);
    child.on("close", (code) => resolve({ code, output }));

    token.onCancellationRequested(() => {
      child.kill();
    });
  });
}

async function runHandler(request, cancellation) {
  if (!controller) return;
  await refreshTests();

  const run = controller.createTestRun(request);
  const requested = collectRequestedTests(request);
  const requestedNames = new Set(requested.map((item) => item.label));

  for (const item of requested) {
    run.enqueued(item);
  }

  try {
    for (const item of requested) {
      run.started(item);
    }

    const startedAt = Date.now();
    const { code, output } = await runPioTest(cancellation);
    const duration = Date.now() - startedAt;
    const jsonResults = await readPlatformioJsonResults(startedAt);
    const parsed = jsonResults.size > 0 ? jsonResults : parseUnityResults(output);

    for (const item of requested) {
      const result = parsed.get(item.label);
      if (!result) {
        const msg = code === 0
          ? "Test was requested, but no Unity result line was found. PlatformIO may have skipped this suite."
          : output;
        run.failed(item, new vscode.TestMessage(msg), duration);
        continue;
      }

      if (result.status === "PASS" || result.status === "PASSED") {
        run.passed(item, result.duration || duration);
      } else if (result.status === "IGNORE" || result.status === "IGNORED" || result.status === "SKIPPED") {
        run.skipped(item);
      } else {
        run.failed(item, new vscode.TestMessage(result.message || output), result.duration || duration);
      }
    }

    // If the user ran a single test, PlatformIO still runs the whole native env.
    // Surface any additional failures so they are not hidden.
    for (const [name, result] of parsed) {
      if (requestedNames.has(name) || !["FAIL", "FAILED", "ERRORED"].includes(result.status)) continue;
      const item = testItemsByName.get(name);
      if (item) {
        run.started(item);
        run.failed(item, new vscode.TestMessage(result.message || output), result.duration || duration);
      }
    }
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    for (const item of requested) {
      run.failed(item, new vscode.TestMessage(message));
    }
  } finally {
    run.end();
  }
}

async function activate(context) {
  controller = vscode.tests.createTestController(
    "platformio-unity-tests",
    "PlatformIO Unity"
  );
  context.subscriptions.push(controller);

  controller.createRunProfile(
    "Run PlatformIO Unity Tests",
    vscode.TestRunProfileKind.Run,
    runHandler,
    true
  );

  context.subscriptions.push(
    vscode.commands.registerCommand("platformioUnityTests.refresh", refreshTests)
  );

  const watcher = vscode.workspace.createFileSystemWatcher("test/**/test_main.cpp");
  watcher.onDidCreate(refreshTests);
  watcher.onDidChange(refreshTests);
  watcher.onDidDelete(refreshTests);
  context.subscriptions.push(watcher);

  await refreshTests();
}

function deactivate() {}

module.exports = {
  activate,
  deactivate,
};
