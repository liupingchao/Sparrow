#ifndef SPARROW_AST_LIST_H_
#define SPARROW_AST_LIST_H_

#include "ast_tree.h"
#include <vector>

/**************************AST内部（非叶子）节点******************************/

class ASTList: public ASTree {

  /*****************内部迭代器类********************/
  template <typename Item>
  class ASTListIterator: public Iterator<Item> {
  public:
    ASTListIterator(const std::vector<Item> &items): items_(items) {};

    void first() override {
      index_ = 0;
    }

    void next() override {
      ++index_;
    }

    bool hasNext() const override {
      return (index_ + 1) < items_.size();
    }

    Item current() const override {
      return items_[index_];
    }

  private:
    const std::vector<Item> &items_;
    size_t index_ = 0;
  };

public:
  ASTList(ASTKind kind, bool ignore);

  //返回第i个子节点
  ASTreePtr child(int i) override;

  //返回子节点数量
  int numChildren() override;

  //返回子节点迭代器
  Iterator<ASTreePtr> iterator() override;

  //返回该节点信息
  std::string info() override;

  //抛出异常
  ObjectPtr eval(EnvPtr env) override;

  std::vector<ASTreePtr>& children();

  bool ignore() const;

protected:
  std::vector<ASTreePtr> children_;
  
  //考虑在没有子节点和子节点个数为1的情况下，该节点是否可以被忽略
  //用于剪枝
  bool ignore_;
};

/**************************元表达式****************************************/
class PostfixAST;
class PrimaryExprAST: public ASTList {
public:
  PrimaryExprAST();
  ObjectPtr eval(EnvPtr env) override;

private:
  ASTreePtr operand();

  //nest表示从外往内数的第几层，如果是最外层，则为0
  std::shared_ptr<PostfixAST> postfix(size_t nest);
  bool hasPostfix(size_t nest);

  //primary表达式可能是一个嵌套调用
  ObjectPtr evalSubExpr(EnvPtr env, size_t nest);
};

/**************************负值表达式*************************************/

class NegativeExprAST: public ASTList {
public:
  NegativeExprAST();
  std::string info() override;
  ObjectPtr eval(EnvPtr env) override;
};

/*****************************二元表达式***********************************/

class BinaryExprAST: public ASTList {
public:
  BinaryExprAST();
  ASTreePtr leftFactor();
  ASTreePtr rightFactor();
  std::string getOperator();
  ObjectPtr eval(EnvPtr env) override;
private:
  //赋值操作
  ObjectPtr assignOp(EnvPtr env, ObjectPtr rightValue);

  //除赋值以外其它操作
  ObjectPtr otherOp(ObjectPtr left, const std::string &op, ObjectPtr right);

  //数字间的运算
  ObjectPtr computeNumber(IntObjectPtr left, const std::string &op, IntObjectPtr right);
private:
  void checkValid();
};

/********************************块**************************************/

class BlockStmntAST: public ASTList {
public:
  BlockStmntAST();
  ObjectPtr eval(EnvPtr env) override;
};
using BlockStmntPtr = std::shared_ptr<BlockStmntAST>;

/******************************if块*************************************/

class IfStmntAST: public ASTList {
public:
  IfStmntAST();
  ASTreePtr condition();
  ASTreePtr thenBlock();
  ASTreePtr elseBlock();
  std::string info() override;
  ObjectPtr eval(EnvPtr env) override;
};

/****************************while块***********************************/

class WhileStmntAST: public ASTList {
public:
  WhileStmntAST();
  ASTreePtr condition();
  ASTreePtr body();
  std::string info() override;
  ObjectPtr eval(EnvPtr env) override;
};

/****************************Null块************************************/

class NullStmntAST: public ASTList {
public:
  NullStmntAST();
  ObjectPtr eval(EnvPtr env) override;
};

/****************************形参列表*********************************/

class ParameterListAST: public ASTList {
public:
  ParameterListAST();
  std::string paramName(size_t i);
  size_t size() const;

  //继承自父类的eval接口废弃，抛出异常
  ObjectPtr eval(__attribute__((unused)) EnvPtr env) override {
    throw ASTEvalException("error call for Parameter List AST eval, abandoned");
  }

  //根据入参设置函数运行时环境
  void eval(EnvPtr funcEnv, EnvPtr callerEnv, const std::vector<ASTreePtr> &args);
};
using ParameterListPtr = std::shared_ptr<ParameterListAST>;

/**************************函数定义**********************************/

class DefStmntAST: public ASTList {
public:
  DefStmntAST();
  std::string funcName();
  ParameterListPtr parameterList();
  BlockStmntPtr block();
  std::string info() override;

  //生成一个函数对象，放入当前的环境中
  //返回空指针
  ObjectPtr eval(EnvPtr env) override;
};

/************************后缀表达式接口*****************************/

class PostfixAST: public ASTList {
public:
  PostfixAST(ASTKind kind, bool ignore): ASTList(kind, ignore) {}

  //Postfix继承自父类的eval方法废弃，会抛出异常，改用其它eval方法
  ObjectPtr eval(__attribute__((unused)) EnvPtr env) override {
    throw ASTEvalException("Postfix AST eval exception, abandoned");
  }
  
  //caller指的是使用这个后缀的对象
  virtual ObjectPtr eval(EnvPtr env, ObjectPtr caller) = 0;
};


/***************************函数实参********************************/

class Arguments: public PostfixAST {
public:
  Arguments();
  size_t size() const;
  
  //函数调用发生在这里
  //caller为函数对象
  ObjectPtr eval(EnvPtr env, ObjectPtr caller) override;

private:
  ObjectPtr invokeNative(EnvPtr env, NativeFuncPtr func);
};

/**************************闭包**********************************/

class LambAST: public ASTList {
public:
  LambAST();
  ParameterListPtr parameterList();
  BlockStmntPtr block();
  std::string info() override;
  ObjectPtr eval(EnvPtr env) override;
};

#endif
