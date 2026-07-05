#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ast.h"
#include "errors.h"
#include "lambda.h"
#include "lexer.h"
#include "parser.h"
#include "peephole.h"
#include "token.h"
#include "visitor.h"

namespace fs = std::filesystem;

static std::string readFile(const std::string &path) {
  std::ifstream f(path, std::ios::binary);
  if (!f.is_open()) throw std::runtime_error("no se pudo abrir el archivo: " + path);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static void dumpTokens(const std::string &source) {
  Lexer lex(source);
  std::cout << "== TOKENS ==\n";
  int idx = 0;
  for (;;) {
    Token t = lex.nextToken();
    std::cout << idx++ << "\t" << t.line << ":" << t.col << "\t"
              << Token::typeName(t.type);
    if (!t.text.empty() && t.type != Token::END) std::cout << "\t'" << t.text << "'";
    std::cout << "\n";
    if (t.type == Token::END) break;
  }
  std::cout << "\n";
}

static bool commandInPath(const std::string &name) {
  const char *pathEnv = std::getenv("PATH");
  if (!pathEnv) return false;
  std::string path(pathEnv);
#ifdef _WIN32
  char sep = ';';
  std::vector<std::string> names = {name, name + ".exe"};
#else
  char sep = ':';
  std::vector<std::string> names = {name};
#endif
  size_t start = 0;
  while (start <= path.size()) {
    size_t end = path.find(sep, start);
    std::string dir = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (!dir.empty()) {
      for (auto &candidate : names) {
        if (fs::exists(fs::path(dir) / candidate)) return true;
      }
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
  return false;
}

static std::string jsonEscape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 16);
  for (unsigned char c : s) {
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if (c < 0x20) {
          static const char *hex = "0123456789abcdef";
          out += "\\u00";
          out += hex[(c >> 4) & 0xF];
          out += hex[c & 0xF];
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  return out;
}

static int countInstr(const std::string &asmText) {
  int n = 0;
  std::istringstream is(asmText);
  std::string line;
  while (std::getline(is, line)) {
    size_t a = line.find_first_not_of(" \t");
    if (a == std::string::npos) continue;
    size_t b = line.find_last_not_of(" \t");
    std::string s = line.substr(a, b - a + 1);
    if (s.empty() || s[0] == '#' || s[0] == '.') continue;
    if (s.back() == ':') continue;
    n++;
  }
  return n;
}

// Quita los marcadores '# .loc N' y conserva el mapa de C# a asm para la web.
static std::string stripLocMarkers(const std::string &asmIn, std::vector<int> &mapOut) {
  std::ostringstream clean;
  std::istringstream is(asmIn);
  std::string line;
  int curSrc = -1;
  while (std::getline(is, line)) {
    size_t a = line.find_first_not_of(" \t");
    std::string t = (a == std::string::npos) ? std::string() : line.substr(a);
    if (t.rfind("# .loc ", 0) == 0) {
      try { curSrc = std::stoi(t.substr(7)); } catch (...) { curSrc = -1; }
      continue;
    }
    // El prólogo de una función no debe heredar la última línea de la anterior.
    if (!t.empty() && t.back() == ':' && t[0] != '.') curSrc = -1;
    clean << line << "\n";
    mapOut.push_back(curSrc);
  }
  return clean.str();
}

static std::string mapPhase(const std::string &phase) {
  if (phase == "lexico") return "lexical";
  if (phase == "sintactico") return "syntax";
  if (phase == "semantico") return "semantic";
  return phase;
}

// Contrato --json: siempre emite un único objeto JSON para backend/server.js.
static int runJson(const std::string &source) {
  auto t0 = std::chrono::steady_clock::now();

  bool success = true;
  bool hasErr = false;
  std::string errPhase = "semantic", errMsg;
  int errLine = 0, errCol = 0;

  std::vector<Token> toks;
  std::string astStr, asmNoOpt, asmOpt;
  std::vector<int> asmSourceMap;
  int folded = 0, peep = 0, instrBefore = 0, instrAfter = 0;

  Program *program = nullptr;
  try {
    {
      Lexer l(source);
      for (;;) {
        Token t = l.nextToken();
        toks.push_back(t);
        if (t.type == Token::END) break;
      }
    }
    // Pipeline principal: parseo -> transformacion de lambdas -> tipos -> AST visible.
    Lexer lexer(source);
    Parser parser(lexer);
    program = parser.parseProgram();
    liftLambdas(program);
    TypeCheckerVisitor checker;
    checker.check(program);
    astStr = printAst(program);

    // Se genera una version sin optimizar para comparar contra la version optimizada.
    {
      GenCodeVisitor g(false);
      g.enableLoc(true);
      g.generar(program);
      asmNoOpt = stripLocMarkers(g.render(), asmSourceMap);
    }
    // Las optimizaciones son independientes: folding sobre AST, orden Sethi-Ullman y peephole.
    {
      ConstantVisitor folder;
      folder.optimize(program);
      folded = folder.folded;
      SethiUllmanVisitor su;
      su.compute(program);
      GenCodeVisitor g(true);
      g.generar(program);
      asmOpt = peephole(g.render(), &peep);
    }
    instrBefore = countInstr(asmNoOpt);
    instrAfter = countInstr(asmOpt);
  } catch (const CompileError &e) {
    success = false;
    hasErr = true;
    errPhase = mapPhase(e.phase);
    errMsg = e.message;
    errLine = e.line;
    errCol = e.col;
  } catch (const std::exception &e) {
    success = false;
    hasErr = true;
    errPhase = "semantic";
    errMsg = e.what();
  }
  delete program;

  auto t1 = std::chrono::steady_clock::now();
  long compileMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

  int removed = instrBefore - instrAfter;
  double reduction =
      instrBefore > 0 ? (removed * 1000.0 / instrBefore) : 0.0;
  reduction = static_cast<long>(reduction + 0.5) / 10.0;

  std::ostringstream o;
  o << "{";
  o << "\"success\":" << (success ? "true" : "false");

  o << ",\"tokens\":[";
  for (size_t i = 0; i < toks.size(); i++) {
    const Token &t = toks[i];
    if (i) o << ",";
    o << "{\"type\":\"" << jsonEscape(Token::typeName(t.type)) << "\""
      << ",\"lexeme\":\"" << jsonEscape(t.type == Token::END ? "" : t.text) << "\""
      << ",\"line\":" << t.line << ",\"col\":" << t.col;
    if (t.type == Token::NUM) {
      try { o << ",\"value\":" << std::stoll(t.text); } catch (...) {}
    }
    o << "}";
  }
  o << "]";

  o << ",\"ast\":\"" << jsonEscape(astStr) << "\"";

  o << ",\"asm\":\"" << jsonEscape(asmNoOpt) << "\"";
  o << ",\"asmOpt\":\"" << jsonEscape(asmOpt) << "\"";

  o << ",\"asmSourceMap\":[";
  for (size_t i = 0; i < asmSourceMap.size(); i++) {
    if (i) o << ",";
    if (asmSourceMap[i] < 0) o << "null"; else o << asmSourceMap[i];
  }
  o << "]";

  o << ",\"optReport\":";
  if (success) {
    std::ostringstream sum;
    sum << folded << " constantes plegadas, " << peep << " lineas peephole; "
        << instrBefore << "->" << instrAfter << " instr (" << reduction << "%)";
    o << "{\"instructionsBefore\":" << instrBefore
      << ",\"instructionsAfter\":" << instrAfter
      << ",\"removed\":" << removed
      << ",\"reductionPercent\":" << reduction
      << ",\"constantFolding\":" << folded
      << ",\"peepholeRemoved\":" << peep
      << ",\"summary\":\"" << jsonEscape(sum.str()) << "\"}";
  } else {
    o << "null";
  }

  o << ",\"errors\":[";
  if (hasErr) {
    o << "{\"phase\":\"" << jsonEscape(errPhase) << "\""
      << ",\"severity\":\"error\""
      << ",\"message\":\"" << jsonEscape(errMsg) << "\""
      << ",\"line\":" << errLine << ",\"col\":" << errCol << "}";
  }
  o << "]";

  o << ",\"metrics\":{\"compileTimeMs\":" << compileMs << "}";

  o << "}";
  std::cout << o.str() << "\n";
  return success ? 0 : 1;
}

int main(int argc, char **argv) {
  std::vector<std::string> files;
  std::string outArg;
  bool optimize = true;
  bool showTokens = false;
  bool showAst = false;
  bool showAstDot = false;
  bool jsonMode = false;

  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--opt") optimize = true;
    else if (a == "--no-opt") optimize = false;
    else if (a == "--tokens") showTokens = true;
    else if (a == "--ast") showAst = true;
    else if (a == "--ast-dot") showAstDot = true;
    else if (a == "--json") jsonMode = true;
    else if (a == "-o" && i + 1 < argc) outArg = argv[++i];
    else if (a.rfind("--", 0) == 0) { std::cerr << "opcion desconocida: " << a << "\n"; return 2; }
    else files.push_back(a);
  }

  if (jsonMode) {
    std::stringstream ss;
    ss << std::cin.rdbuf();
    return runJson(ss.str());
  }

  if (files.empty()) {
    std::cerr << "Uso: " << argv[0]
              << " <archivo.cs> [--opt|--no-opt] [--tokens] [--ast] [--ast-dot] [--json] [-o salida.s]\n";
    return 2;
  }

  const std::string inputPath = files[0];
  std::string source;
  try {
    source = readFile(inputPath);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }

  if (showTokens) {
    try {
      dumpTokens(source);
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      return 1;
    }
  }

  Program *program = nullptr;
  try {
    Lexer lexer(source);
    Parser parser(lexer);
    program = parser.parseProgram();
    liftLambdas(program);
    TypeCheckerVisitor checker;
    checker.check(program);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    delete program;
    return 1;
  }

  if (showAst) {
    std::cout << "== AST ==\n" << printAst(program) << "\n";
  }

  if (showAstDot) {
    try {
      std::string stem = fs::path(inputPath).stem().string();
      std::error_code ec;
      fs::create_directories("outputs/ast", ec);
      std::string dotPath = "outputs/ast/" + stem + "_ast.dot";
      std::string pngPath = "outputs/ast/" + stem + "_ast.png";
      std::ofstream dotfile(dotPath, std::ios::binary);
      if (!dotfile.is_open()) throw std::runtime_error("no se pudo crear: " + dotPath);
      dotfile << astToDot(program);
      dotfile.close();
      std::cout << "AST DOT generado -> " << dotPath << "\n";
      if (commandInPath("dot")) {
        std::string cmd = "dot -Tpng \"" + dotPath + "\" -o \"" + pngPath + "\"";
        int rc = std::system(cmd.c_str());
        if (rc == 0) std::cout << "AST PNG generado -> " << pngPath << "\n";
        else std::cout << "No se pudo generar el PNG con Graphviz dot.\n";
      } else {
        std::cout << "No se pudo generar el PNG con Graphviz dot.\n";
      }
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
      delete program;
      return 1;
    }
  }

  std::string outputPath = outArg;
  bool defaultOutput = outputPath.empty();
  if (outputPath.empty()) {
    std::string stem = fs::path(inputPath).stem().string();
    std::error_code ec;
    fs::create_directories("outputs/asm", ec);
    fs::create_directories("outputs/bin", ec);
    outputPath = "outputs/asm/" + stem + ".s";
  }

  int foldedCount = 0, peepRemoved = 0;
  try {
    if (optimize) {
      ConstantVisitor folder;
      folder.optimize(program);
      foldedCount = folder.folded;
      SethiUllmanVisitor su;
      su.compute(program);
    }

    GenCodeVisitor codegen(optimize);
    codegen.generar(program);
    std::string asmText = codegen.render();

    if (optimize) asmText = peephole(asmText, &peepRemoved);

    std::error_code ec;
    fs::path outParent = fs::path(outputPath).parent_path();
    if (!outParent.empty()) fs::create_directories(outParent, ec);

    std::ofstream outfile(outputPath, std::ios::binary);
    if (!outfile.is_open()) throw std::runtime_error("no se pudo crear: " + outputPath);
    outfile << asmText;
    outfile.close();
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    delete program;
    return 1;
  }

  delete program;

  std::string bin = defaultOutput
                        ? ("outputs/bin/" + fs::path(inputPath).stem().string())
                        : fs::path(outputPath).replace_extension("").string();
  std::cout << "Compilacion exitosa -> " << outputPath
            << (optimize ? "  [--opt]" : "  [--no-opt]") << "\n";
  if (optimize)
    std::cout << "Optimizacion: " << foldedCount << " constantes plegadas, "
              << peepRemoved << " lineas eliminadas por peephole.\n";
  std::cout << "Para ensamblar y ejecutar:\n";
  std::cout << "  g++ -no-pie " << outputPath << " -o " << bin << "\n";
  std::cout << "  ./" << bin << "\n";
  return 0;
}
