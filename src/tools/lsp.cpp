#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include "../include/lexer.hpp"
#include "../include/parser.hpp"
#include "../include/resolver.hpp"
#include "../include/version.hpp"

// Simple JSON helper for LSP
namespace json {
    inline std::string escape(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
        }
        return out;
    }
}

class SulfurLSP {
public:
    void run() {
        while (std::cin) {
            std::string line;
            int contentLength = -1;

            // Read headers
            while (std::getline(std::cin, line)) {
                // Trim trailing \r
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.empty()) break; // end of headers

                if (line.rfind("Content-Length: ", 0) == 0) {
                    contentLength = std::stoi(line.substr(16));
                }
            }

            if (contentLength <= 0) continue;

            // Read JSON body
            std::vector<char> buf(contentLength);
            std::cin.read(buf.data(), contentLength);
            std::string body(buf.data(), contentLength);

            handleRequest(body);
        }
    }

private:
    void sendResponse(const std::string& jsonContent) {
        std::cout << "Content-Length: " << jsonContent.size() << "\r\n\r\n"
                  << jsonContent << std::flush;
    }

    void handleRequest(const std::string& raw) {
        // Primitive parser for LSP JSON-RPC methods
        if (raw.find("\"method\":\"initialize\"") != std::string::npos) {
            handleInitialize(raw);
        } else if (raw.find("\"method\":\"textDocument/didOpen\"") != std::string::npos) {
            handleDidOpen(raw);
        } else if (raw.find("\"method\":\"textDocument/didChange\"") != std::string::npos) {
            handleDidChange(raw);
        } else if (raw.find("\"method\":\"textDocument/completion\"") != std::string::npos) {
            handleCompletion(raw);
        } else if (raw.find("\"method\":\"textDocument/hover\"") != std::string::npos) {
            handleHover(raw);
        } else if (raw.find("\"method\":\"shutdown\"") != std::string::npos) {
            sendResponse("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":null}");
        }
    }

    void handleInitialize(const std::string& raw) {
        // Extract id if possible
        std::string id = "1";
        size_t idPos = raw.find("\"id\":");
        if (idPos != std::string::npos) {
            size_t end = raw.find_first_of(",}", idPos);
            id = raw.substr(idPos + 5, end - (idPos + 5));
        }

        std::string resp = "{"
            "\"jsonrpc\":\"2.0\","
            "\"id\":" + id + ","
            "\"result\":{"
                "\"capabilities\":{"
                    "\"textDocumentSync\":1,"
                    "\"completionProvider\":{\"resolveProvider\":false,\"triggerCharacters\":[\".\",\":\"]},"
                    "\"hoverProvider\":true,"
                    "\"documentSymbolProvider\":true"
                "},"
                "\"serverInfo\":{\"name\":\"sulfur-lsp\",\"version\":\"" + std::string(__SULFUR_VERSION__) + "\"}"
            "}"
        "}";
        sendResponse(resp);
    }

    void handleDidOpen(const std::string& raw) {
        // Parse URI and Text for diagnostics
        validateDocument(raw);
    }

    void handleDidChange(const std::string& raw) {
        validateDocument(raw);
    }

    void validateDocument(const std::string& raw) {
        // Perform fast lex + parse to emit diagnostics
        size_t textPos = raw.find("\"text\":\"");
        if (textPos == std::string::npos) return;
        
        // Extract source
        size_t textStart = textPos + 8;
        size_t textEnd = raw.rfind("\"");
        if (textEnd <= textStart) return;
        std::string src = raw.substr(textStart, textEnd - textStart);

        // Extract URI
        std::string uri = "file:///temp.sfpp";
        size_t uriPos = raw.find("\"uri\":\"");
        if (uriPos != std::string::npos) {
            size_t uEnd = raw.find("\"", uriPos + 7);
            uri = raw.substr(uriPos + 7, uEnd - (uriPos + 7));
        }

        std::string diagJson = "[]";
        try {
            Lexer lex(src, uri);
            auto toks = lex.tokenize();
            Parser parser(std::move(toks));
            auto ast = parser.parse();
            Resolver resolver;
            resolver.resolve(ast);
        } catch (const ParseError& e) {
            int line = std::max(0, e.line - 1);
            diagJson = "[{"
                "\"range\":{\"start\":{\"line\":" + std::to_string(line) + ",\"character\":0},"
                          "\"end\":{\"line\":" + std::to_string(line) + ",\"character\":100}},"
                "\"severity\":1,"
                "\"source\":\"sulfur-lsp\","
                "\"message\":\"" + json::escape(e.what()) + "\""
            "}]";
        } catch (const LexError& e) {
            int line = std::max(0, e.line - 1);
            diagJson = "[{"
                "\"range\":{\"start\":{\"line\":" + std::to_string(line) + ",\"character\":0},"
                          "\"end\":{\"line\":" + std::to_string(line) + ",\"character\":100}},"
                "\"severity\":1,"
                "\"source\":\"sulfur-lsp\","
                "\"message\":\"" + json::escape(e.what()) + "\""
            "}]";
        } catch (...) {}

        std::string notif = "{"
            "\"jsonrpc\":\"2.0\","
            "\"method\":\"textDocument/publishDiagnostics\","
            "\"params\":{"
                "\"uri\":\"" + uri + "\","
                "\"diagnostics\":" + diagJson + ""
            "}"
        "}";
        sendResponse(notif);
    }

    void handleCompletion(const std::string& raw) {
        std::string id = "1";
        size_t idPos = raw.find("\"id\":");
        if (idPos != std::string::npos) {
            size_t end = raw.find_first_of(",}", idPos);
            id = raw.substr(idPos + 5, end - (idPos + 5));
        }

        std::string items = "["
            "{\"label\":\"fn\",\"kind\":14,\"detail\":\"Function declaration\",\"insertText\":\"fn ${1:name}(${2:params}) {\\n\\t$0\\n}\"},"
            "{\"label\":\"let\",\"kind\":14,\"detail\":\"Immutable variable\",\"insertText\":\"let ${1:name} = ${2:value};\"},"
            "{\"label\":\"var\",\"kind\":14,\"detail\":\"Mutable variable\",\"insertText\":\"var ${1:name} = ${2:value};\"},"
            "{\"label\":\"class\",\"kind\":7,\"detail\":\"Class definition\",\"insertText\":\"class ${1:Name} {\\n\\t+1>init;\\n\\tfn init(${2:params}) {\\n\\t\\t$0\\n\\t}\\n}\"},"
            "{\"label\":\"struct\",\"kind\":7,\"detail\":\"Struct definition\",\"insertText\":\"struct ${1:Name} {\\n\\t${2:fields}\\n}\"},"
            "{\"label\":\"match\",\"kind\":14,\"detail\":\"Pattern matching\",\"insertText\":\"match (${1:expr}) {\\n\\t${2:pattern} => { $0 }\\n\\t_ => {}\\n}\"},"
            "{\"label\":\"defer\",\"kind\":14,\"detail\":\"Defer block (LIFO cleanup)\",\"insertText\":\"defer {\\n\\t$0\\n}\"},"
            "{\"label\":\"try\",\"kind\":14,\"detail\":\"Try-catch block\",\"insertText\":\"try {\\n\\t$0\\n} catch (e) {\\n\\t\\n}\"},"
            "{\"label\":\"import\",\"kind\":9,\"detail\":\"Import module\",\"insertText\":\"import ${1:module} as ${2:alias};\"},"
            "{\"label\":\"int_64\",\"kind\":25,\"detail\":\"64-bit signed integer\"},"
            "{\"label\":\"float_64\",\"kind\":25,\"detail\":\"64-bit IEEE float\"},"
            "{\"label\":\"complex_128\",\"kind\":25,\"detail\":\"128-bit complex number\"},"
            "{\"label\":\"str\",\"kind\":25,\"detail\":\"UTF-8 String type\"},"
            "{\"label\":\"bool\",\"kind\":25,\"detail\":\"Boolean type\"}"
        "]";

        std::string resp = "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + items + "}";
        sendResponse(resp);
    }

    void handleHover(const std::string& raw) {
        std::string id = "1";
        size_t idPos = raw.find("\"id\":");
        if (idPos != std::string::npos) {
            size_t end = raw.find_first_of(",}", idPos);
            id = raw.substr(idPos + 5, end - (idPos + 5));
        }

        std::string resp = "{"
            "\"jsonrpc\":\"2.0\","
            "\"id\":" + id + ","
            "\"result\":{"
                "\"contents\":{\"kind\":\"markdown\",\"value\":\"**Sulfur++ Modern Systems Language**\\n\\n*Write Easy, Perform Fast*\"}"
            "}"
        "}";
        sendResponse(resp);
    }
};

int main() {
    SulfurLSP server;
    server.run();
    return 0;
}
