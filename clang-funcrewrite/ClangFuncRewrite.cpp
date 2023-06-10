#include <sstream>
#include <string>

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

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
 public:
  MyASTVisitor(Rewriter &R) : TheRewriter(R) {}

  bool VisitFunctionDecl(FunctionDecl *f) {
    // only function definitions (with bodies), not declarations
    if (f->hasBody()) {
      Stmt *FuncBody = f->getBody();

      SourceLocation slStart = FuncBody->getBeginLoc();
      SourceLocation slEnd = FuncBody->getEndLoc();

      std::stringstream fbBefore;
      fbBefore << "\n#if 0\n";
      TheRewriter.InsertText(slStart, fbBefore.str(), true, true);

      std::stringstream fbEnd;
      fbEnd << "}\n#endif\n";
      TheRewriter.ReplaceText(slEnd, fbEnd.str());
    }

    return true;
  }

 private:
  Rewriter &TheRewriter;
};

class MyASTConsumer : public ASTConsumer {
 public:
  MyASTConsumer(Rewriter &R) : Visitor(R) {}

  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      Visitor.TraverseDecl(*b);
      (*b)->dump();
    }
    return true;
  }

 private:
  MyASTVisitor Visitor;
};

// for each source file provided to the tool, a new FrontendAction is created
class MyFrontendAction : public ASTFrontendAction {
 public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    SourceManager &SM = TheRewriter.getSourceMgr();
    llvm::errs() << "** EndSourceFileAction for: " << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";

    // now emit the rewritten buffer.
    TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
    llvm::errs() << "** creating AST consumer for: " << file << "\n";
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

 private:
  Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
  llvm::Expected<CommonOptionsParser> op = CommonOptionsParser::create(argc, argv, ToolingSampleCategory);
  ClangTool Tool(op.get().getCompilations(), op.get().getSourcePathList());
  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
