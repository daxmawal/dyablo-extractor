#include <iostream>

#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

class MyDependencyCollector
    : public RecursiveASTVisitor<MyDependencyCollector> {
 public:
  explicit MyDependencyCollector(ASTContext* Context) : Context(Context) {}

  bool VisitDeclRefExpr(DeclRefExpr* DRE)
  {
    if (const FunctionDecl* FD = dyn_cast<FunctionDecl>(DRE->getDecl())) {
      if (FD->getBuiltinID() == 0 && FD->hasBody()) {
        Dependencies.insert(FD);
      }
    }
    if (const VarDecl* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
      if (VD->hasGlobalStorage()) {
        Dependencies.insert(VD);
      }
    }
    return true;
  }

  bool VisitCXXRecordDecl(CXXRecordDecl* CRD)
  {
    if (CRD->isThisDeclarationADefinition()) {
      Dependencies.insert(CRD);
    }
    return true;
  }

  std::set<const Decl*> getDependencies() const { return Dependencies; }

 private:
  ASTContext* Context;
  std::set<const Decl*> Dependencies;
};

class FunctionPrinter : public MatchFinder::MatchCallback {
 public:
  void run(const MatchFinder::MatchResult& Result) override
  {
    if (const FunctionDecl* FD =
            Result.Nodes.getNodeAs<FunctionDecl>("targetFunc")) {
      if (FD->hasBody()) {
        llvm::outs() << "=== Function ===\n";
        FD->print(llvm::outs());

        MyDependencyCollector Collector(Result.Context);
        Collector.TraverseDecl(const_cast<FunctionDecl*>(FD));

        for (const Decl* D : Collector.getDependencies()) {
          llvm::outs() << "\n=== Dependency ===\n";
          D->print(llvm::outs());
        }

        std::cout << "\n";
      }
    }
  }
};


int
main(int argc, const char** argv)
{
  llvm::cl::OptionCategory ToolCategory("function-extractor");

  auto ExpectedParser = CommonOptionsParser::create(argc, argv, ToolCategory);
  if (!ExpectedParser) {
    llvm::errs() << "Error while parsing command-line arguments: "
                 << llvm::toString(ExpectedParser.takeError()) << "\n";
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();

  ClangTool Tool(
      OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

  FunctionPrinter Printer;
  MatchFinder Finder;
  Finder.addMatcher(
      functionDecl(hasName("say_hello")).bind("targetFunc"), &Printer);

  return Tool.run(newFrontendActionFactory(&Finder).get());
}
