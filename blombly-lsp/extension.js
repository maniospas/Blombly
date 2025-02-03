// npm install
// sudo npm install -g @vscode/vsce
// sudo npm install --save-dev @vscode/test-electron

// TO PACAKGE AND RUN
// vsce package
// code --install-extension blombly-lsp-0.0.1.vsix


const vscode = require("vscode");
const { LanguageClient, LanguageClientOptions, ServerOptions } = require("vscode-languageclient/node");
const path = require("path");
const { execSync, spawn } = require("child_process");
const fs = require("fs");

let client;
const SERVER_PORT = 2087; 

function isPortFree(port) {
  try {
    execSync(`lsof -i :${port}`);
    return false; // Port is in use
  } catch (error) {
    return true; // Port is free
  }
}

function killPortProcess(port) {
  try {
    let processId;
    
    if (process.platform === "darwin" || process.platform === "linux") {
      processId = execSync(`lsof -t -i:${port}`).toString().trim();
    } else if (process.platform === "win32") {
      processId = execSync(`netstat -ano | findstr :${port}`).toString().match(/\d+$/);
      processId = processId ? processId[0] : null;
    }

    if (processId) {
      execSync(`kill -9 ${processId}`);
      vscode.window.showInformationMessage(`Closed process on port ${port}`);
    }
  } catch (error) {
    // Port not in use, nothing to kill
  }
}

function activate(context) {
  const extensionDir = context.globalStorageUri.fsPath;
  const venvPath = path.join(extensionDir, "blombly_venv");
  const venvPython = path.join(venvPath, "bin", "python3");
  const serverPath = context.asAbsolutePath(path.join(".", "lsp.py"));

  if (!fs.existsSync(path.join(venvPath, "bin", "activate"))) {
    vscode.window.showInformationMessage("Creating virtual environment for Blombly LSP...");
    try {
      execSync(`python3 -m venv "${venvPath}"`, { stdio: "inherit" });
    } catch (error) {
      vscode.window.showErrorMessage("Failed to create virtual environment.");
      return;
    }
  }

  try {
    vscode.window.showInformationMessage("Installing dependencies for Blombly LSP...");
    execSync(`"${venvPython}" -m pip install --upgrade pip`, { stdio: "inherit" });
    execSync(`"${venvPython}" -m pip install pygls`, { stdio: "inherit" });

    vscode.window.showInformationMessage("Blombly LSP dependencies installed successfully!");
  } catch (error) {
    vscode.window.showErrorMessage("Failed to install dependencies in the virtual environment.");
    return;
  }

  if (!isPortFree(SERVER_PORT)) {
    vscode.window.showErrorMessage(`Port ${SERVER_PORT} is already in use. Trying to free it.`);
    killPortProcess(SERVER_PORT);
  }

  const serverProcess = spawn(venvPython, [serverPath], { cwd: path.dirname(serverPath) });

  serverProcess.stdout.on("data", (data) => {
    console.log(`Blombly LSP: ${data}`);
  });

  serverProcess.stderr.on("data", (data) => {
    console.error(`Blombly LSP Error: ${data}`);
  });

  serverProcess.on("exit", (code) => {
    console.log(`Blombly LSP exited with code ${code}`);
  });

  const serverOptions = {
    run: { command: venvPython, args: [serverPath], options: { stdio: 'pipe' } },
    debug: { command: venvPython, args: [serverPath, "--debug"], options: { stdio: 'pipe' } }
  };
  

  const clientOptions = {
    documentSelector: [{ scheme: "file", language: "blombly" }],
    outputChannel: vscode.window.createOutputChannel("Blombly LSP"),
  };

  try {
    client = new LanguageClient("blombly-lsp", "Blombly LSP", serverOptions, clientOptions);
    client.outputChannel.appendLine("Starting Blombly LSP...");
    vscode.window.showInformationMessage("Blombly LSP is starting!");

    client.start();

    client.onReady().then(() => {
      vscode.window.showInformationMessage("Blombly LSP is fully ready!");
    }).catch((error) => {
      vscode.window.showErrorMessage("Blombly LSP failed to start properly: " + error);
      killPortProcess(SERVER_PORT);
    });

  } catch (error) {
    vscode.window.showErrorMessage("Failed to initialize Blombly LSP.");
    killPortProcess(SERVER_PORT);
  }
}

function deactivate() {
  vscode.window.showInformationMessage("Deactivating Blombly LSP.");
  if (!client) return;
  killPortProcess(SERVER_PORT);
  return client.stop();
}

module.exports = { activate, deactivate };
