//
// Created by pinping on 2023/9/6.
//

#include <iostream>
#include <clang/AST/AST.h>
#include <clang/AST/DeclObjC.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>

using namespace clang;
using namespace std;
using namespace llvm;
using namespace clang::ast_matchers;

namespace MKPlugin {
class MKMatchCallback : public MatchFinder::MatchCallback {
private:
  CompilerInstance &CI;

  bool isUserSourceCode(const string filename) {
    if (filename.empty()) return false;
    if (filename.find("/Applications/Xcode.app/") == 0) return false;
    return true;
  }

  bool shouldUseCopy(const string typeStr) {
    if (typeStr.find("NSArray") != string::npos ||
        typeStr.find("NSString") != string::npos ||
        typeStr.find("NSDictionary") != string::npos ) {
      return true;
    }
    return false;
  }

public:
  MKMatchCallback(CompilerInstance &CI):CI(CI){}

  void run(const MatchFinder::MatchResult &Result) override {
    const ObjCPropertyDecl *propertyDecl = Result.Nodes.getNodeAs<ObjCPropertyDecl>("objcPropertyDecl");
    if (propertyDecl) {
      string fileName = CI.getSourceManager().getFilename(propertyDecl->getSourceRange().getBegin()).str();
      if (isUserSourceCode(fileName)) {
        string classTypeStr = propertyDecl->getType().getAsString();
        ObjCPropertyAttribute::Kind kind = propertyDecl->getPropertyAttributes();
        if (shouldUseCopy(classTypeStr) && !(kind & ObjCPropertyAttribute::Kind::kind_copy)) {
          DiagnosticsEngine &engine = CI.getDiagnostics();
          unsigned int diagID = engine.getCustomDiagID(clang::DiagnosticsEngine::Warning, "不可变对象的setter语义推荐使用copy修饰");
          engine.Report(propertyDecl->getBeginLoc(), diagID);
        }
      }
    }

    const ObjCInterfaceDecl *objcInterfaceDecl = Result.Nodes.getNodeAs<ObjCInterfaceDecl>("objcInterfaceDecl");
    if (objcInterfaceDecl) {
      string filename = CI.getSourceManager().getFilename(objcInterfaceDecl->getSourceRange().getBegin()).str();
      DiagnosticsEngine &engine = CI.getDiagnostics();
      if (isUserSourceCode(filename)) {
        StringRef name = objcInterfaceDecl->getName();
        char c = name[0];
        if (isLowercase(c)) {
          unsigned diagID = engine.getCustomDiagID(DiagnosticsEngine::Warning, "类名首字母不应该使用小写字母");
          SourceLocation location = objcInterfaceDecl->getLocation();
          std::string tempName = name.str();
          tempName[0] = toUppercase(c);
          StringRef replacement(tempName);
          SourceLocation nameStart = objcInterfaceDecl->getLocation();
          SourceLocation nameEnd = nameStart.getLocWithOffset(name.size());
          FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
          engine.Report(location, diagID).AddFixItHint(fixItHint);
        }

        size_t pos = objcInterfaceDecl->getName().find('_');
        if (pos != StringRef::npos) {
          std::string tempName = name.str();
          std::string::iterator end_pos = std::remove(tempName.begin(), tempName.end(), '_');
          tempName.erase(end_pos, tempName.end());
          StringRef replacement(tempName);
          SourceLocation nameStart = objcInterfaceDecl->getLocation();
          SourceLocation nameEnd = nameStart.getLocWithOffset(name.size());
          FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(nameStart, nameEnd), replacement);
          SourceLocation loc = objcInterfaceDecl->getLocation().getLocWithOffset(pos);
          engine.Report(loc, engine.getCustomDiagID(clang::DiagnosticsEngine::Error, "类名中不允许包含下划线_")).AddFixItHint(fixItHint);
        }
      }
    }
  }
};

// 继承 ASTConsumer 实现自定义逻辑
class MKConsumer : public ASTConsumer {
private:
  MatchFinder matchFinder;
  MKMatchCallback callback;
public:
  MKConsumer(CompilerInstance &CI):callback(CI) {
    matchFinder.addMatcher(objcPropertyDecl().bind("objcPropertyDecl"), &callback);//属性声明
    matchFinder.addMatcher(objcInterfaceDecl().bind("objcInterfaceDecl"), &callback);//类声明
  }
  void HandleTranslationUnit(ASTContext &Ctx) override {
    matchFinder.matchAST(Ctx);
  }
};

// 继承 PluginASTAction 实现自定义的逻辑
class MKASTAction : public PluginASTAction {
public:
  bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &arg) override {
    return true;
  }
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override {
    return unique_ptr<MKConsumer>(new MKConsumer(CI));
  }
};
}

// 注册插件
static FrontendPluginRegistry::Add<MKPlugin::MKASTAction> X("TestPlugin", "这里是插件的描述信息");
