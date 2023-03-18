#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

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
using namespace std;

static llvm::cl::OptionCategory MyOptionCategory("MyOptions");
static llvm::cl::opt<std::string> OutputFilename("o", 
        llvm::cl::desc("Specify output filename that contains stmt:type"), 
        llvm::cl::value_desc("output_filename"), llvm::cl::cat(MyOptionCategory));

LangOptions MyLangOpts; 
SourceManager *ptrMySourceMgr;
Rewriter MyRewriter;

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor() {}
  bool VisitStmt(Stmt *s) {
    // Print a current statement and its type 
    std::string str1;
    llvm::raw_string_ostream os(str1);
    s->printPretty(os, NULL, MyLangOpts);

    puts("-----------------");
    printf("%s\n", os.str().c_str());
    printf("TYPE:%s\n", s->getStmtClassName());
    fflush(stdout);
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *f) { // Print function name
    llvm::outs() << "*********************************\n";
    llvm::outs() << "*** FUNCTION NAME:" <<  f->getName() << '\n';
    llvm::outs() << "*********************************\n";
    return true;
  }
};

class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(): Visitor() {} //initialize MyASTVisitor

  virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
      // Travel each function declaration using MyASTVisitor
      Visitor.TraverseDecl(*b);
    }
    return true;
  }

private:
    MyASTVisitor Visitor;
};

class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
 
  void EndSourceFileAction() override {
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(
                  CompilerInstance &CI, StringRef file) override {                                         

    MyLangOpts = CI.getLangOpts();
    ptrMySourceMgr= &(CI.getSourceManager());
    MyRewriter= Rewriter(*ptrMySourceMgr, MyLangOpts);

    return std::make_unique<MyASTConsumer>();
  }
};

int main(int argc, const char **argv) {  
  llvm::Expected<clang::tooling::CommonOptionsParser> ExpectedOp = CommonOptionsParser::create(argc, argv, MyOptionCategory);
  if (!ExpectedOp) {
    llvm::errs() << ExpectedOp.takeError();
    return 1;
  }

  CommonOptionsParser &op = ExpectedOp.get();

  ClangTool Tool(op.getCompilations(), op.getSourcePathList());
  int ret = Tool.run(newFrontendActionFactory<MyFrontendAction>().get());

  /*
  Rewriter sample. Save changed target code into output.txt if any 
  const RewriteBuffer *RewriteBuf = MyRewriter.getRewriteBufferFor
     ((*ptrMySourceMgr).getMainFileID());
  ofstream out_file ("output.txt");
  out_file << string(RewriteBuf->begin(), RewriteBuf->end());
  out_file.close();
  */

  return ret;
}
