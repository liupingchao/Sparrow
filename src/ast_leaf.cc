#include "ast_leaf.h"

#include "symbols.h"
#include "ast_list.h"

/************************AST叶节点，没有子节点**************************/

ASTLeaf::ASTLeaf(ASTKind kind, TokenPtr token): ASTree(kind), token_(token) {}

ASTreePtr ASTLeaf::child(__attribute__((unused)) int i) {
  return nullptr;
}

int ASTLeaf::numChildren() {
  return 0;
}

Iterator<ASTreePtr> ASTLeaf::iterator() {
  throw ASTException("error call: no children for AST leaf");
}

std::string ASTLeaf::info() {
  return token_->getText();
}

ObjectPtr ASTLeaf::eval(__attribute__((unused)) EnvPtr env) {
  throw ASTEvalException("error call: not evalable for AST leaf");
}

TokenPtr ASTLeaf::getToken() const {
  return token_;
}

void ASTLeaf::setIndex(size_t index) {
  index_ = index;
}

size_t ASTLeaf::getIndex() const {
  return index_;
}

/*********************IntToken对应的叶子节点***************************/

IntTokenAST::IntTokenAST(TokenPtr token): ASTLeaf(ASTKind::LEAF_INT, token) {}

std::string IntTokenAST::info() {
  return std::to_string(getValue());
}

ObjectPtr IntTokenAST::eval(__attribute__((unused)) EnvPtr env) {
  return std::make_shared<IntObject>(getValue());
}

int IntTokenAST::getValue() const {
  return std::static_pointer_cast<IntToken>(token_)->getValue();
}

void IntTokenAST::preProcess(__attribute__((unused))SymbolsPtr symbols) {
  index_ = g_IntSymbols->getIndex(getValue());
}

void IntTokenAST::compile() {
  auto codes = FuncObject::getCurrCompilingFunc()->getCodes();
  codes->iconst(index_);
}

/*********************FloatToken对应的叶子节点************************/

FloatTokenAST::FloatTokenAST(TokenPtr token): ASTLeaf(ASTKind::LEAF_FLOAT, token) {}

std::string FloatTokenAST::info() {
  return std::to_string(getValue());
}

ObjectPtr FloatTokenAST::eval(__attribute__((unused))EnvPtr env) {
  return std::make_shared<FloatObject>(getValue());
}

double FloatTokenAST::getValue() const {
  return std::static_pointer_cast<FloatToken>(token_)->getValue();
}

void FloatTokenAST::preProcess(__attribute__((unused))SymbolsPtr symbols) {
  index_ = g_FloatSymbols->getIndex(getValue());
}

void FloatTokenAST::compile() {
  auto codes = FuncObject::getCurrCompilingFunc()->getCodes();
  codes->fconst(index_);
}

/*********************IdToken对应的叶子节点***************************/

IdTokenAST::IdTokenAST(TokenPtr token): ASTLeaf(ASTKind::LEAF_Id, token) {}

std::string IdTokenAST::info() {
  return getId();
}

ObjectPtr IdTokenAST::eval(EnvPtr env) {
  if (kind_ == IdKind::LOCAL) {
    //MyDebugger::print("local: " + getId(), __FILE__, __LINE__);
    if (index_ < 0)
      throw ASTEvalException("invalid index for local variable: " + getId());
    return env->get(index_);
  }
  else if (kind_ == IdKind::CLOSURE) {
    //闭包变量的环境的上层环境是函数运行时局部环境，即使函数结束运行了
    //还是存储着，直接引用即可
    EnvPtr outerFuncEnv = env->getOuterEnv();
    if (outerFuncEnv == nullptr)
      throw ASTEvalException("null outer function env");
    //MyDebugger::print("closure: " + getId(), __FILE__, __LINE__);
    return outerFuncEnv->get(index_);
  }
  else {
    //MyDebugger::print("global: " + getId(), __FILE__, __LINE__);
    return env->get(getId());
  } 
}

void IdTokenAST::assign(EnvPtr env, ObjectPtr value) {
  if (kind_ == IdKind::LOCAL) {
    env->put(index_, value);
  }
  else if (kind_ == IdKind::CLOSURE) {
    EnvPtr outerFuncEnv = env->getOuterEnv();
    if (outerFuncEnv == nullptr)
      throw ASTEvalException("null outer function env");
    outerFuncEnv->put(index_, value);
  }
  else {
    env->put(getId(), value);
  }
}

std::string IdTokenAST::getId() const {
  return token_->getText();
}

void IdTokenAST::preProcess(SymbolsPtr symbols) {
  int result = symbols->getRuntimeIndex(getId());
  if (result == -1) {
    kind_ = IdKind::GLOBAL;
  }
  else if (result >= 0) {
    kind_ = IdKind::LOCAL;
    index_ = result;
  }
  else {
    kind_ = IdKind::CLOSURE;
    index_ = result + 2;    //该变量在上层环境的位置
  }
}

void IdTokenAST::compile() {
  auto func = FuncObject::getCurrCompilingFunc();
  auto codes = func->getCodes();
  if (kind_ == IdKind::GLOBAL) {
    unsigned index = func->getRuntimeIndex(getId());
    codes->gload(index);
    //MyDebugger::print(getId() + " " + std::to_string(index), __FILE__, __LINE__);
  } 
  else if (kind_ == IdKind::CLOSURE) {
    codes->cload(index_);
    //MyDebugger::print(getId() + " " + std::to_string(index_), __FILE__, __LINE__);
  }
  else {
    codes->load(index_);
    //MyDebugger::print(getId() + " " + std::to_string(index_), __FILE__, __LINE__);
  }
}

void IdTokenAST::complieAssign() {
  auto func = FuncObject::getCurrCompilingFunc();
  auto codes = func->getCodes();
  if (kind_ == IdKind::GLOBAL) {
    unsigned index = func->getRuntimeIndex(getId());
    codes->gstore(index);
  } 
  else if (kind_ == IdKind::CLOSURE) {
    codes->cstore(index_);
  }
  else {
    codes->store(index_);
  }
}

void IdTokenAST::compileAsRawString() {
  auto func = FuncObject::getCurrCompilingFunc();
  index_ = func->getRuntimeIndex(getId());
  auto codes_ = func->getCodes();
  codes_->rawString(index_);
}

/*********************StrToken对应的叶子节点*************************/

StrTokenAST::StrTokenAST(TokenPtr token): ASTLeaf(ASTKind::LEAF_STR, token) {}

std::string StrTokenAST::info() {
  return getContent();
}

ObjectPtr StrTokenAST::eval(__attribute__((unused)) EnvPtr env) {
  return std::make_shared<StrObject>(getContent());
}

std::string StrTokenAST::getContent() const {
  return token_->getText();
}

void StrTokenAST::preProcess(__attribute__((unused))SymbolsPtr symbols) {
  index_ = g_StrSymbols->getIndex(getContent());
}

void StrTokenAST::compile() {
  auto codes = FuncObject::getCurrCompilingFunc()->getCodes();
  codes->sconst(index_);
}
