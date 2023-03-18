#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

int main()
{
  CompilerInstance ci;
  ci.createDiagnostics(0, NULL);                                    // create DiagnosticsEngine
  ci.createFileManager();                                           // create FileManager
  ci.createSourceManager(ci.getFileManager());                      // create SourceManager
  ci.createPreprocessor();                                          // create Preprocessor
  const FileEntry *pFile = ci.getFileManager().getFile("hello.c");  
  ci.getSourceManager().createMainFileID(pFile);
  ci.getPreprocessor().EnterMainSourceFile();
  ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(), &ci.getPreprocessor());
  Token tok;
  do {
      ci.getPreprocessor().Lex(tok);
      if( ci.getDiagnostics().hasErrorOccurred()) {
        break;
      }
        
      ci.getPreprocessor().DumpToken(tok);
      std::cerr << std::endl; 
  } while (tok.isNot(clang::tok::eof));

  ci.getDiagnosticClient().EndSourceFile();

  return 0;
}