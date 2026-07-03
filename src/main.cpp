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

int main(int argc, char **argv) {
  std::vector<std::string> files;
  std::string outArg;
  bool optimize = true;
  bool showTokens = false;
  bool showAst = false;
  bool showAstDot = false;

  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--opt") optimize = true;
    else if (a == "--no-opt") optimize = false;
    else if (a == "--tokens") showTokens = true;
    else if (a == "--ast") showAst = true;
    else if (a == "--ast-dot") showAstDot = true;
    else if (a == "-o" && i + 1 < argc) outArg = argv[++i];
    else if (a.rfind("--", 0) == 0) { std::cerr << "opcion desconocida: " << a << "\n"; return 2; }
    else files.push_back(a);
  }

  if (files.empty()) {
    std::cerr << "Uso: " << argv[0]
              << " <archivo.cs> [--opt|--no-opt] [--tokens] [--ast] [--ast-dot] [-o salida.s]\n";
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
