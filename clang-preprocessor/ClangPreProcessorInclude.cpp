#include <iostream>
#include <memory>

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

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/PreprocessorOptions.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace clang::frontend;

int main(int argc, char *argv[])
{
  if (argc != 2) {
    llvm::errs() << "usage: " << argv[0] << " <input-file>\n";
    return 1;
  }

  // create a CompilerInstance and set the target platform to the host platform
  CompilerInstance ci;
  ci.createDiagnostics();                                    // create DiagnosticsEngine
  
  auto to = std::make_shared<TargetOptions>();
  to->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *targetInfo = TargetInfo::CreateTargetInfo(ci.getDiagnostics(), to);
  ci.setTarget(targetInfo);
  

  // create a FileManager and a SourceManager
  ci.createFileManager();                                    // create FileManager
  ci.createSourceManager(ci.getFileManager());               // create SourceManager

  // set up HeaderSearchOptions
  HeaderSearchOptions &hso = ci.getHeaderSearchOpts();

  // add the system include path
  hso.AddPath("/usr/include", System, false, false);
  hso.AddPath("/usr/local/include", System, false, false);
  hso.AddPath("/usr/local/lib/gcc/x86_64-pc-linux-gnu/7.5.0/include", System, false, false);

  // add the current directory as a user include path
  hso.AddPath(".", Angled, false, false);

  ci.createPreprocessor(TU_Complete);                        // create Preprocessor

  llvm::ErrorOr<const clang::FileEntry*> pFile = ci.getFileManager().getFile(argv[1]);
  SourceManager &SourceMgr = ci.getSourceManager();
  SourceMgr.setMainFileID(SourceMgr.createFileID(pFile.get(), SourceLocation(), SrcMgr::C_User));

  // ci.getSourceManager().createMainFileID(pFile.get());

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