#include <stdio.h>
#include <exception>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

class StackFrame {
 private:
  std::map<Decl *, int> mVars_;
  std::map<Stmt *, int> mExprs_;
  Stmt *mPC_;

 public:
  StackFrame() = default;

  bool hasDecl(Decl *decl) { return mVars_.find(decl) != mVars_.end(); }

  void bindDecl(Decl *decl, int val) { mVars_[decl] = val; }

  int getDeclVal(Decl *decl) {
    assert(mVars_.find(decl) != mVars_.end());
    return mVars_.find(decl)->second;
  }

  bool hasStmt(Stmt *stmt) { return mExprs_.find(stmt) != mExprs_.end(); }

  void bindStmt(Stmt *stmt, int val) { mExprs_[stmt] = val; }

  int getStmtVal(Stmt *stmt) {
    if (!hasStmt(stmt)) {
      llvm::outs() << "cannot find the value of stmt:\n";
      stmt->dump();
    }

    assert(mExprs_.find(stmt) != mExprs_.end());
    return mExprs_[stmt];
  }

  void setPC(Stmt *stmt) { mPC_ = stmt; }
  Stmt *getPC() { return mPC_; }
};

class Heap {
 public:
  using HeapAddr = int;

 private:
  static const HeapAddr kInitHeapSize = sizeof(int) * 1024;

  void *mHeapPtr_;
  HeapAddr mOffset_;

  inline int *actualAddr(HeapAddr addr) {
    assert(addr <= kInitHeapSize);
    return (int *)((char *)mHeapPtr_ + addr);
  }

 public:
  Heap() : mHeapPtr_(malloc(kInitHeapSize)), mOffset_(0) {}
  ~Heap() { free(mHeapPtr_); }

  HeapAddr Malloc(int size) {
    HeapAddr start = mOffset_;
    mOffset_ += size;
    llvm::outs() << "allocate size: " << size << " return address: " << start
                 << " still have: " << kInitHeapSize - mOffset_ << "\n";
    assert(mOffset_ <= kInitHeapSize);
    return start;
  }

  void Free(HeapAddr addr) {}

  void Update(HeapAddr addr, int val) {
    int *ptr = actualAddr(addr);
    *ptr = val;
    llvm::outs() << "Update *" << addr << " -> " << val << "\n";
  }

  int get(HeapAddr addr) {
    int *ptr = actualAddr(addr);
    return *ptr;
  }

  static int getPtrSize() { return sizeof(HeapAddr); }

  static int step2Size(int step) { return step * getPtrSize(); }
};

class ReturnException : public std::exception {
  int mRet_;

 public:
  explicit ReturnException(int ret) : mRet_(ret) {}

  int getRetVal() const { return mRet_; }
};

class Array {
  int mScope_;
  std::vector<int> mArr_;

 public:
  Array(int sz, int scope) : mArr_(sz), mScope_(scope) {}
  void set(int i, int val) {
    assert(i <= mArr_.size());
    mArr_[i] = val;
  }
  int get(int i) {
    assert(i <= mArr_.size());
    return mArr_[i];
  }
};

class InterpreterVisitor;

class Environment {
 private:
  EvaluatedExprVisitor<InterpreterVisitor> *mInterpreter_;

  Heap mHeap_;
  std::vector<StackFrame> mStack_;
  std::vector<Array> mArrays_;

  FunctionDecl *mFree_;  /// Declartions to the built-in functions
  FunctionDecl *mMalloc_;
  FunctionDecl *mInput_;
  FunctionDecl *mOutput_;

  FunctionDecl *mEntry_;

 public:
  void setInterpreter(EvaluatedExprVisitor<InterpreterVisitor> *visitor) { this->mInterpreter_ = visitor; }
  void stackPop() { mStack_.pop_back(); }

  StackFrame &stackTop() { return mStack_.back(); }

  StackFrame &globalScope() { return mStack_[0]; }

  void bindDecl(Decl *decl, int val) {
    if (stackTop().hasDecl(decl)) {
      stackTop().bindDecl(decl, val);
    } else {
      // it should be a global variable
      llvm::outs() << "bind global decl\n";
      globalScope().bindDecl(decl, val);
    }
  }

  int getDeclVal(Decl *decl) {
    if (stackTop().hasDecl(decl)) {
      return stackTop().getDeclVal(decl);
    }  
    // it should be a global variable
    // llvm::outs() << "get global decl\n";
    // decl->dump();
    return globalScope().getDeclVal(decl);
  }

  void bindStmt(Stmt *stmt, int val) { stackTop().bindStmt(stmt, val); }

  int getStmtVal(Stmt *stmt) { return stackTop().getStmtVal(stmt); }

  static const int kScH001 = 11217991;
  /// Get the declartions to the built-in functions
  Environment() : mFree_(nullptr), mMalloc_(nullptr), mInput_(nullptr), mOutput_(nullptr), mEntry_(nullptr) {}

  void init(TranslationUnitDecl *unit) {
    mStack_.emplace_back(StackFrame());
    for (TranslationUnitDecl::decl_iterator i = unit->decls_begin(), e = unit->decls_end(); i != e; ++i) {
      if (auto *fdecl = dyn_cast<FunctionDecl>(*i)) {
        if (fdecl->getName().equals("FREE")) {
          mFree_ = fdecl;
        } else if (fdecl->getName().equals("MALLOC")) {
          mMalloc_ = fdecl;
        } else if (fdecl->getName().equals("GET")) {
          mInput_ = fdecl;
        } else if (fdecl->getName().equals("PRINT")) {
          mOutput_ = fdecl;
        } else if (fdecl->getName().equals("main")) {
          mEntry_ = fdecl;
        }
      } else if (auto *vdecl = dyn_cast<VarDecl>(*i)) {
        /// global variable?
        this->handleVarDecl(vdecl);
      }
    }
  }

  FunctionDecl *getEntry() { return mEntry_; }

  void uop(UnaryOperator *uop) {
    auto op_code = uop->getOpcode();
    int val = stackTop().getStmtVal(uop->getSubExpr());
    switch (op_code) {
      case UO_Minus:
        val = -val;
        break;
      case UO_Plus:
        break;
      case UO_Not:
        val = ~val;
        break;
      case UO_LNot:
        val = !val;
        break;
      case UO_Deref:
        val = mHeap_.get(val);
        break;
      default:
        llvm::outs() << "Below uop is not supported: \n";
        uop->dump();
        break;
    }
    stackTop().bindStmt(uop, val);
  }

  int handleAdditive(int opCode, Expr *left, Expr *right, int lval, int rval) {
    /// handle
    auto ltype = left->getType();
    auto rtype = right->getType();
    bool l_is_ptr = ltype->isPointerType();
    bool r_is_ptr = rtype->isPointerType();
    if (l_is_ptr && r_is_ptr) {
      assert(opCode == BO_Sub);
      return (lval - rval) / Heap::getPtrSize();
    }

    if (l_is_ptr) {
      rval = Heap::step2Size(rval);
    } else if (r_is_ptr) {
      lval = Heap::step2Size(lval);
    }

    if (opCode == BO_Add) {
      return lval + rval;
    }

    return lval - rval;
  }

  void binop(BinaryOperator *bop) {
    Expr *left = bop->getLHS();
    Expr *right = bop->getRHS();

    int rval = stackTop().getStmtVal(right);

    auto op_code = bop->getOpcode();
    int res = 0;
    if (bop->isAssignmentOp()) {
      stackTop().bindStmt(left, rval);
      stackTop().bindStmt(bop, rval);

      if (auto *declexpr = dyn_cast<DeclRefExpr>(left)) {
        Decl *decl = declexpr->getFoundDecl();
        this->bindDecl(decl, rval);
      } else if (auto *arrsub = dyn_cast<ArraySubscriptExpr>(left)) {
        auto &arr = getArray(arrsub);
        auto idx = getArrayIdx(arrsub);
        arr.set(idx, rval);
        this->bindStmt(arrsub, rval);
      } else if (auto *uop = dyn_cast<UnaryOperator>(left)) {
        assert(uop->getOpcode() == UO_Deref);
        int addr = stackTop().getStmtVal(uop->getSubExpr());
        mHeap_.Update(addr, rval);
        this->bindStmt(uop, rval);  // `*ptr = VAL;` should return VAL
      } else {
        llvm::outs() << "below assignment(LHS) is not supported\n";
        left->dump();
      }
    } else if (bop->isAdditiveOp()) {
      int lval = stackTop().getStmtVal(left);
      res = handleAdditive(op_code, left, right, lval, rval);
      stackTop().bindStmt(bop, res);
    } else if (bop->isMultiplicativeOp()) {
      int lval = stackTop().getStmtVal(left);
      if (op_code == BO_Mul) {
        res = lval * rval;
      } else {
        res = lval % rval;
      }
      stackTop().bindStmt(bop, res);
    } else if (bop->isComparisonOp()) {
      int lval = stackTop().getStmtVal(left);
      int val = kScH001;
      switch (op_code) {
        case BO_LT:
          val = (lval < rval);
          break;
        case BO_GT:
          val = (lval > rval);
          break;
        case BO_LE:
          val = (lval <= rval);
          break;
        case BO_GE:
          val = (lval >= rval);
          break;
        case BO_EQ:
          val = (lval == rval);
          break;
        case BO_NE:
          val = (lval != rval);
          break;
      }
      // llvm::outs() << "op: " << op << "val " << val << "\n";
      stackTop().bindStmt(bop, val);
    }

    else {
      llvm::outs() << "Below Binary op is Not Supported\n";
      bop->dump();
    }
  }

  void parm(ParmVarDecl *parmdecl, int val) { stackTop().bindDecl(parmdecl, val); }

  /// use by global & local
  void handleVarDecl(VarDecl *vardecl) {
    auto type_info = vardecl->getType();
    if (type_info->isArrayType()) {
      // array type
      assert(type_info->isConstantArrayType());
      const auto *array_type = vardecl->getType()->getAsArrayTypeUnsafe();
      const auto *carray_type = dyn_cast<const ConstantArrayType>(array_type);
      int sz = carray_type->getSize().getSExtValue();
      assert(sz > 0);
      llvm::outs() << "init a array with size: " << carray_type->getSize() << "\n";
      mArrays_.emplace_back(sz, mStack_.size());
      stackTop().bindDecl(vardecl, mArrays_.size() - 1);
    }

    int val = 0;
    Expr *expr = vardecl->getInit();
    if (expr != nullptr) {
      mInterpreter_->Visit(expr);
      val = stackTop().getStmtVal(expr);
    }

    stackTop().bindDecl(vardecl, val);
  }

  void decl(DeclStmt *declstmt) {
    for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end(); it != ie; ++it) {
      Decl *decl = *it;
      if (auto *vardecl = dyn_cast<VarDecl>(decl)) {
        handleVarDecl(vardecl);
        // llvm::outs() << "opaque data: " << vardecl->getTypeSourceInfo()->getTypeLoc().getOpaqueData() << "\n";
      }
    }
  }

  Array &getArray(ArraySubscriptExpr *arrsubexpr) {
    int arrayID = stackTop().getStmtVal(arrsubexpr->getBase());
    assert(arrayID < mArrays_.size());
    return mArrays_[arrayID];
  }

  int getArrayIdx(ArraySubscriptExpr *arrsubexpr) { return stackTop().getStmtVal(arrsubexpr->getIdx()); }

  void arraysub(ArraySubscriptExpr *arrsubexpr) {
    auto &arr = getArray(arrsubexpr);
    int idx = getArrayIdx(arrsubexpr);
    int res = arr.get(idx);
    // llvm::outs() << "arr[" << idx << "]-> " << res << "\n";
    stackTop().bindStmt(arrsubexpr, res);
  }

  static bool isValidDeclRefType(DeclRefExpr *declref) {
    auto tp = declref->getType();
    return tp->isIntegerType() || tp->isArrayType() || tp->isPointerType();
  }

  bool isBuiltInDecl(DeclRefExpr *declref) {
    const Decl *decl = declref->getReferencedDeclOfCallee();
    return declref->getType()->isFunctionType() &&
           (decl == mInput_ || decl == mOutput_ || decl == mMalloc_ || decl == mFree_);
  }

  void declref(DeclRefExpr *declref) {
    stackTop().setPC(declref);
    if (isValidDeclRefType(declref)) {
      Decl *decl = declref->getFoundDecl();

      int val = this->getDeclVal(decl);
      stackTop().bindStmt(declref, val);
    } else if (!isBuiltInDecl(declref) && !declref->getType()->isFunctionType()) {
      llvm::outs() << "Below declref is not supported:\n";
      declref->dump();
    }
  }

  void cast(CastExpr *castexpr) {
    stackTop().setPC(castexpr);
    if (castexpr->getType()->isIntegerType()) {
      Expr *expr = castexpr->getSubExpr();
      int val = stackTop().getStmtVal(expr);
      stackTop().bindStmt(castexpr, val);
    }
  }

  bool call(CallExpr *callexpr) {
    bool not_builtin = false;
    stackTop().setPC(callexpr);
    int val = 0;
    FunctionDecl *callee = callexpr->getDirectCallee();
    if (callee == mInput_) {
      llvm::outs() << "please input an integer value: ";
      scanf("%d", &val);
      stackTop().bindStmt(callexpr, val);
    } else if (callee == mOutput_) {
      Expr *decl = callexpr->getArg(0);
      val = stackTop().getStmtVal(decl);
      llvm::errs() << val;
    } else if (callee == mMalloc_) {
      Expr *decl = callexpr->getArg(0);
      val = stackTop().getStmtVal(decl);
      int addr = mHeap_.Malloc(val);
      stackTop().bindStmt(callexpr, addr);
    } else if (callee == mFree_) {
      Expr *decl = callexpr->getArg(0);
      val = stackTop().getStmtVal(decl);
      mHeap_.Free(val);
    } else {
      // llvm::outs() << "function call\n";
      not_builtin = true;
      /// first we get the arguments from caller frame
      std::vector<int> args;
      Expr **expr_list = callexpr->getArgs();
      for (int i = 0; i < callexpr->getNumArgs(); i++) {
        int val = stackTop().getStmtVal(expr_list[i]);
        args.push_back(val);
      }

      /// You could add your code here for Function call Return
      mStack_.emplace_back(StackFrame());  // push frame
      // define parameter list
      assert(callee->getNumParams() == callexpr->getNumArgs());
      for (int i = 0; i < callee->getNumParams(); i++) {
        this->parm(callee->getParamDecl(i), args[i]);
      }

      stackTop().setPC(callee->getBody());
    }
    return not_builtin;
  }

  void retrn(ReturnStmt *retstmt) {
    stackTop().setPC(retstmt);
    int val = stackTop().getStmtVal(retstmt->getRetValue());
    // llvm::outs() << "return val: " << val << "\n";
    throw ReturnException(val);
  }
};