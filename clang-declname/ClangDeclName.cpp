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

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

class CustomASTConsumer : public ASTConsumer {
public:
  CustomASTConsumer () :  ASTConsumer() { }
  virtual ~ CustomASTConsumer () { }
  virtual bool HandleTopLevelDecl(DeclGroupRef decls)
  {
    clang::DeclGroupRef::iterator it;

    for (it = decls.begin(); it != decls.end(); it++) {
      clang::VarDecl *vd = llvm::dyn_cast<clang::VarDecl>(*it);
      if (vd) {
        std::cout << vd->getDeclName().getAsString() << std::endl;
      }
    }
    return true;
  }
};

int main() {
  CompilerInstance ci;
  ci.createDiagnostics();
  

  auto to = std::make_shared<TargetOptions>();
  to->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *tinfo = TargetInfo::CreateTargetInfo(ci.getDiagnostics(), to);
  ci.setTarget(tinfo);

  ci.createFileManager();
  ci.createSourceManager(ci.getFileManager());
  ci.createPreprocessor(TU_Complete);
  ci.createASTContext();
  CustomASTConsumer *astConsumer = new CustomASTConsumer();
  // ci.setASTConsumer(astConsumer);
  
  llvm::ErrorOr<const clang::FileEntry*> pFile = ci.getFileManager().getFile("example.cpp");
  SourceManager &SourceMgr = ci.getSourceManager();
  SourceMgr.setMainFileID(SourceMgr.createFileID(pFile.get(), SourceLocation(), SrcMgr::C_User));

  ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(), &ci.getPreprocessor());
  clang::ParseAST(ci.getPreprocessor(), astConsumer, ci.getASTContext());
  ci.getDiagnosticClient().EndSourceFile();

  return 0;
}