#include "env.h"
#include "ast_list.h"

/*****************************函数 类型******************************/
FuncObject::FuncObject(const std::string &functionName, 
    std::shared_ptr<ParameterListAST> params, 
    std::shared_ptr<BlockStmntAST> block, EnvPtr env):
  Object(ObjKind::Func), funcName_(functionName), params_(params), block_(block), env_(env) {}

std::shared_ptr<ParameterListAST> FuncObject::params() const {
  return params_;
}

std::shared_ptr<BlockStmntAST> FuncObject::block() const {
  return block_; 
}

EnvPtr FuncObject::runtimeEnv() const {
  return std::make_shared<Environment>(env_);
}

/***************************类元信息*********************************/
ClassInfo::ClassInfo(std::shared_ptr<ClassStmntAST> stmnt, EnvPtr env):
  Object(ObjKind::Class_Info), definition_(stmnt), env_(env) {

  ObjectPtr obj = env->get(stmnt->superClassName());
  if (obj == nullptr)   
    return;
  else if (obj->kind_ == ObjKind::Class_Info)
    superClass_ = std::static_pointer_cast<ClassInfo>(obj);
  else
    throw EnvException("unknown super class: " + stmnt->superClassName());
}

std::string ClassInfo::name() {
  return definition_->name();
}

ClassInfoPtr ClassInfo::superClass() {
  return superClass_;
}

std::shared_ptr<ClassBodyAST> ClassInfo::body() {
  return definition_->body();
}

EnvPtr ClassInfo::getEnvitonment() {
  return env_;
}

std::string ClassInfo::info() {
  return "<class: " + name() + ">";
}

/**************************对象*************************************/

ClassInstance::ClassInstance(EnvPtr env): Object(ObjKind::Class_Instance), env_(env){}

void ClassInstance::write(const std::string &member, ObjectPtr value) {
  if (checkAccessValid(member))
    env_->put(member, value);
}

ObjectPtr ClassInstance::read(const std::string &member) {
  checkAccessValid(member);
  return env_->get(member);
}

bool ClassInstance::checkAccessValid(const std::string &member) {
  if (env_->isExistInCurrentEnv(member))
    return true;
  else
    throw EnvException("Access " + member + " error");
}

std::string ClassInstance::info() {
  return "<object>";
}

/*************************定长数组***********************************/

Array::Array(size_t i): Object(ObjKind::Array) {
  array_ = std::vector<ObjectPtr>(i, nullptr);
}

void Array::set(size_t i, ObjectPtr obj) {
  if (i >= array_.size())
    throw OutOfIndexException();
  else
    array_[i] = obj;
}

ObjectPtr Array::get(size_t i) {
  if (i >= array_.size())
    throw OutOfIndexException();
  else
    return array_[i];
}

std::string Array::info() {
  return "<array with size " + std::to_string(array_.size()) + ">";
}

/*******************************环境********************************/
Environment::Environment() = default;

Environment::Environment(EnvPtr outer): outerEnv_(outer) {}

void Environment::setOuterEnv(EnvPtr outer) {
  outerEnv_ = outer;
}

ObjectPtr Environment::get(const std::string &name) {
  auto env = locateEnv(name);
  if (env == nullptr)
    return nullptr;
  else 
    return env->env_[name];
}

void Environment::put(const std::string &name, ObjectPtr obj) {
  auto env = locateEnv(name);
  if (env == nullptr)
    env = this->shared_from_this();
  env->putNew(name, obj);
}

bool Environment::isExistInCurrentEnv(const std::string &name) {
  return env_.find(name) != env_.end();
}

EnvPtr Environment::locateEnv(const std::string &name) {
  if (env_.find(name) != env_.end()) 
    return this->shared_from_this();
  else if (outerEnv_ == nullptr)
    return nullptr;
  else
    return outerEnv_->locateEnv(name);
}

void Environment::putNew(const std::string &name, ObjectPtr obj) {
  env_[name] = obj;
}
