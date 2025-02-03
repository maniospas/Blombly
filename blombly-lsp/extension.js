// npm install
// sudo npm install -g @vscode/vsce
// sudo npm install --save-dev @vscode/test-electron

// TO PACAKGE AND RUN
// vsce package
// code --install-extension blombly-lsp-0.0.1.vsix

const net = require('net');
const vscode = require('vscode');
const { LanguageClient, LanguageClientOptions } = require('vscode-languageclient/node');

let client;

function activate(context) {
  const serverOptions = () => {
    return new Promise((resolve, reject) => {
      const socket = net.connect({ host: '127.0.0.1', port: 2087 }, () => {
        resolve({
          reader: socket,
          writer: socket
        });
      });
      socket.on('error', (err) => {
        reject(err);
      });
    });
  };
  const clientOptions = {documentSelector: [{ scheme: 'file', language: 'blombly' }]};
  client = new LanguageClient(
    'blombly-lsp',
    'Blombly LSP',
    serverOptions,
    clientOptions
  );
  client.start();
}

function deactivate() {
  if(!client) {return undefined;}
  return client.stop();
}

exports.activate = activate;
exports.deactivate = deactivate;
