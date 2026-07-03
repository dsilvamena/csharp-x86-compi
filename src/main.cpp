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

int main(int argc, char **argv) {
  std::vector<std::string> files;
  std::string outArg;
  bool optimize = true;
  bool showTokens = false;
  bool showAst = false;

  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--opt") optimize = true;
    else if (a == "--no-opt") optimize = false;
    else if (a == "--tokens") showTokens = true;
    else if (a == "--ast") showAst = true;
    else if (a == "-o" && i + 1 < argc) outArg = argv[++i];
    else if (a.rfind("--", 0) == 0) { std::cerr << "opcion desconocida: " << a << "\n"; return 2; }
    else files.push_back(a);
  }

  if (files.empty()) {
    std::cerr << "Uso: " << argv[0]
              << " <archivo.cs> [--opt|--no-opt] [--tokens] [--ast] [-o salida.s]\n";
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

  std::string outputPath = outArg;
  if (outputPath.empty()) {
    std::string stem = fs::path(inputPath).stem().string();
    std::error_code ec;
    fs::create_directories("outputs", ec);
    outputPath = "outputs/" + stem + ".s";
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

  std::string bin = fs::path(outputPath).replace_extension("").string();
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
