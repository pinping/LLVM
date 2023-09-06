//
// Created by pinping on 2023/9/6.
//
#include <iostream>
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

using namespace clang;
using namespace std;
using namespace llvm;
using namespace clang::ast_matchers;

namespace TestPlugin {
class TestHandler : public MatchFinder::MatchCallback{
private:
  CompilerInstance &ci;

public:
  TestHandler(CompilerInstance &ci) :ci(ci) {}

  //判断是否是用户源文件
  bool isUserSourceCode(const string filename) {
    //文件名不为空
    if (filename.empty()) return  false;
    //非xcode中的源码都认为是用户的
    if (filename.find("/Applications/Xcode.app/") == 0) return false;
    return  true;
  }

  // 代码检查的回调方法
  void run(const MatchFinder::MatchResult &Result) {

    // 检查类名(Interface)，不能带有下划线
    if (const ObjCInterfaceDecl *decl = Result.Nodes.getNodeAs<ObjCInterfaceDecl>("ObjCInterfaceDecl")) {
      string filename = ci.getSourceManager().getFilename(decl->getSourceRange().getBegin()).str();
      if ( !isUserSourceCode(filename) ) return;
      size_t pos = decl->getName().find('_');
      if (pos != StringRef::npos) {
        DiagnosticsEngine &D = ci.getDiagnostics();
        // 获取位置
        SourceLocation loc = decl->getLocation().getLocWithOffset(pos);
        D.Report(loc, D.getCustomDiagID(DiagnosticsEngine::Warning, "TestPlugin：类名中不能带有下划线"));
      }
    }
    // 检查变量(Interface)，不能带有下划线
    if (const VarDecl *decl = Result.Nodes.getNodeAs<VarDecl>("VarDecl")) {
      string filename = ci.getSourceManager().getFilename(decl->getSourceRange().getBegin()).str();
      if ( !isUserSourceCode(filename) ) return;
      size_t pos = decl->getName().find('_');
      if (pos != StringRef::npos && pos != 0) {
        DiagnosticsEngine &D = ci.getDiagnostics();
        SourceLocation loc = decl->getLocation().getLocWithOffset(pos);
        D.Report(loc, D.getCustomDiagID(DiagnosticsEngine::Warning, "TestPlugin2：请使用驼峰命名，不建议使用下划线"));
      }
    }
  }
};


// 定义语法树的接受事件
class TestASTConsumer: public ASTConsumer{
private:
  MatchFinder matcher;
  TestHandler handler;

public:
  TestASTConsumer(CompilerInstance &ci) :handler(ci) {
    matcher.addMatcher(objcInterfaceDecl().bind("ObjCInterfaceDecl"), &handler);
    matcher.addMatcher(varDecl().bind("VarDecl"), &handler);
    matcher.addMatcher(objcMethodDecl().bind("ObjCMethodDecl"), &handler);
  }
  void HandleTranslationUnit(ASTContext &Ctx) {
    printf("TestPlugin1: All ASTs has parsed.");
    DiagnosticsEngine &D = Ctx.getDiagnostics();
    // 在编译log中可以看到
    D.Report(D.getCustomDiagID(DiagnosticsEngine::Warning, "TestPlugin警告提示"));
    D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "TestPlugin错误信息"));
    matcher.matchAST(Ctx);
  }
};


// 定义触发插件的动作
class TestAction : public PluginASTAction{
public:
  unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                            StringRef InFile){
    return unique_ptr<TestASTConsumer> (new TestASTConsumer(CI));

  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &arg){
    return true;
  }
};
}


// 告知clang,注册一个新的plugin
static FrontendPluginRegistry::Add<TestPlugin::TestAction> X("TestPlugin", "Test a new Plugin");
// X 变量名，可随便写，也可以写自己有意思的名称
// TestPlugin  插件名称，️很重要，这个是对外的名称
// Test a new Plugin  插件备注