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

int main() {
  CompilerInstance ci;
  ci.createDiagnostics(0,NULL);
  TargetOptions to;
  to.Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *tin = TargetInfo::CreateTargetInfo(ci.getDiagnostics(), to);
  ci.setTarget(tin);
  ci.createFileManager();
  ci.createSourceManager(ci.getFileManager());
  ci.createPreprocessor();
  ci.createASTContext();
  CustomASTConsumer *astConsumer = new CustomASTConsumer ();
  ci.setASTConsumer(astConsumer);
  const FileEntry *file = ci.getFileManager().getFile("hello.c");
  
  ci.getSourceManager().createMainFileID(file);
  ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(), &ci.getPreprocessor());
  clang::ParseAST(ci.getPreprocessor(), astConsumer, ci.getASTContext());
  ci.getDiagnosticClient().EndSourceFile();

  return 0;
}