#include <iostream>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
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
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"

#include "Environment.h"

using namespace clang;

class InterpreterVisitor : public EvaluatedExprVisitor<InterpreterVisitor> {
 public:
  explicit InterpreterVisitor(const ASTContext &context, Environment *env) : EvaluatedExprVisitor(context), mEnv_(env) {
    env->setInterpreter(this);
  }

  virtual ~InterpreterVisitor() = default;

  virtual void VisitBinaryOperator(BinaryOperator *bop) {
    bop->dump();
    VisitStmt(bop);
    mEnv_->binop(bop);
  }

  virtual void VisitUnaryOperator(UnaryOperator *uop) {
    uop->dump();
    VisitStmt(uop);
    mEnv_->uop(uop);
  }

  virtual void VisitIntegerLiteral(IntegerLiteral *il) {
    il->dump();
    int val = il->getValue().getSExtValue();
    mEnv_->bindStmt(il, val);
  }

  virtual void VisitDeclRefExpr(DeclRefExpr *expr) {
    expr->dump();
    VisitStmt(expr);
    mEnv_->declref(expr);
  }

  virtual void VisitCastExpr(CastExpr *expr) {
    expr->dump();
    VisitStmt(expr);
    mEnv_->cast(expr);
  }

  virtual void VisitCallExpr(CallExpr *call) {
    call->dump();
    VisitStmt(call);
    bool not_builtin = mEnv_->call(call);
    try {
      if (not_builtin) {
        VisitStmt(mEnv_->stackTop().getPC());
      }
    } catch (ReturnException &e) {
      int ret_val = e.getRetVal();
      mEnv_->stackPop();
      mEnv_->stackTop().bindStmt(call, ret_val);
    }
  }

  virtual void VisitDeclStmt(DeclStmt *declstmt) {
    declstmt->dump();
    mEnv_->decl(declstmt);
  }

  int getChildrenSize(Stmt *stmt) {
    int i = 0;
    for (auto *c : stmt->children()) {
      i++;
    }
    return i;
  }

  virtual void VisitArraySubscriptExpr(ArraySubscriptExpr *arrsubexpr) {
    arrsubexpr->dump();
    // llvm::outs() << "children size: " << getChildrenSize(arrsubexpr) << "\n";
    VisitStmt(arrsubexpr);
    mEnv_->arraysub(arrsubexpr);
  }

  virtual void VisitReturnStmt(ReturnStmt *retstmt) {
    retstmt->dump();
    VisitStmt(retstmt);
    mEnv_->retrn(retstmt);
  }

  virtual void VisitIfStmt(IfStmt *ifstmt) {
    ifstmt->dump();
    Expr *cond_expr = ifstmt->getCond();
    this->Visit(cond_expr);
    int cond = mEnv_->stackTop().getStmtVal(cond_expr);
    if (cond) {
      // llvm::outs() << "then branch\n";
      if (ifstmt->getThen()) {
        this->Visit(ifstmt->getThen());
      }
    } else {
      if (ifstmt->getElse()) {
        this->Visit(ifstmt->getElse());
      }
      // llvm::outs() << "else branch\n";
    }
  }

  virtual void VisitWhileStmt(WhileStmt *wstmt) {
    wstmt->dump();
    Expr *cond_expr = wstmt->getCond();
    do {
      this->Visit(cond_expr);
      int cond = mEnv_->getStmtVal(cond_expr);
      if (!cond) {
        break;
      }
      this->Visit(wstmt->getBody());
    } while (true);
  }

  virtual void VisitForStmt(ForStmt *fstmt) {
    fstmt->dump();
    Stmt *initstmt = fstmt->getInit();
    if (initstmt) {
      this->Visit(initstmt);
    }
    Expr *cond_expr = fstmt->getCond();
    do {
      this->Visit(cond_expr);
      int cond = mEnv_->getStmtVal(cond_expr);
      if (!cond) {
        break;
      }
      this->Visit(fstmt->getBody());
      this->Visit(fstmt->getInc());
    } while (true);
  }

  virtual void VisitCStyleCastExpr(CStyleCastExpr *ccastexpr) {
    ccastexpr->dump();
    this->VisitStmt(ccastexpr);
    stealBindingFromChild(ccastexpr);
  }

  virtual void VisitImplicitCastExpr(ImplicitCastExpr *icastexpr) {
    icastexpr->dump();
    this->VisitStmt(icastexpr);
    stealBindingFromChild(icastexpr);
  }

  virtual void VisitParenExpr(ParenExpr *parenexpr) {
    parenexpr->dump();
    this->VisitStmt(parenexpr);
    stealBindingFromChild(parenexpr);
  }

  /// for some AST(e.g., ImplicitCastExpr, CStyleCastExpr), we need to have their "value" binding
  /// so we steal the value binding from their children. usually, they have only one child
  void stealBindingFromChild(Stmt *parent) {
    parent->dump();
    Stmt *stmt = nullptr;
    for (auto *c : parent->children()) {
      stmt = c;
      break;
    }

    if (stmt) {
      if (mEnv_->stackTop().hasStmt(stmt)) {
        mEnv_->bindStmt(parent, mEnv_->stackTop().getStmtVal(stmt));
        // llvm::outs() << "succ\n";
        // stmt->dump();
        return;
      }
    }
  }

  virtual void VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *uexpr) {
    uexpr->dump();
    this->VisitStmt(uexpr);
    /// we assume the op must be `sizeof`
    // uexpr->getExprStmt()->dump();
    auto arg_type = uexpr->getArgumentTypeInfo()->getType();
    int sz = 0;
    if (arg_type->isPointerType()) {
      sz = sizeof(Heap::HeapAddr);
    } else if (arg_type->isIntegerType()) {
      sz = sizeof(int);
    } else {
      llvm::outs() << "Unknown Type:\n";
      arg_type.dump();
      throw std::exception();
    }
    mEnv_->bindStmt(uexpr, sz);
  }

 private:
  Environment *mEnv_;
};

class InterpreterConsumer : public ASTConsumer {
 public:
  explicit InterpreterConsumer(const ASTContext &context) : mVisitor_(context, &mEnv_) {}
  ~InterpreterConsumer() override = default;

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    TranslationUnitDecl *decl = Context.getTranslationUnitDecl();
    mEnv_.init(decl);
    FunctionDecl *entry = mEnv_.getEntry();
    try {
      mVisitor_.VisitStmt(entry->getBody());
    } catch (ReturnException &e) {
      if (e.getRetVal() != 0) {
        llvm::outs() << "main exit with a non-zero code!\n";
      }
    }
  }

 private:
  Environment mEnv_;
  InterpreterVisitor mVisitor_;
};

class InterpreterFrontendAction : public ASTFrontendAction {
 public:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &ci,
                                                        llvm::StringRef /*InFile*/) override {
    return std::unique_ptr<clang::ASTConsumer>(new InterpreterConsumer(ci.getASTContext()));
  }
};

int main(int argc, char **argv) {
  if (argc > 1) {
    clang::tooling::runToolOnCode(std::unique_ptr<clang::FrontendAction>(new InterpreterFrontendAction), argv[1]);
  }
}