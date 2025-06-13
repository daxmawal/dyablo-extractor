#include <iostream>

#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Path.h"

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
        SourceManager& SM = Context->getSourceManager();
        if (SM.isWrittenInMainFile(FD->getLocation())) {
          Dependencies.insert(FD);
        } else {
          std::string File =
              llvm::sys::path::filename(SM.getFilename(FD->getLocation()))
                  .str();
          HeaderIncludes.insert(File);
        }
      }
    }
    if (const VarDecl* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
      if (VD->hasGlobalStorage()) {
        SourceManager& SM = Context->getSourceManager();
        if (SM.isWrittenInMainFile(VD->getLocation())) {
          Dependencies.insert(VD);
        } else {
          std::string File =
              llvm::sys::path::filename(SM.getFilename(VD->getLocation()))
                  .str();
          HeaderIncludes.insert(File);
        }
      }
    }
    return true;
  }

  bool VisitCXXRecordDecl(CXXRecordDecl* CRD)
  {
    if (CRD->isThisDeclarationADefinition()) {
      SourceManager& SM = Context->getSourceManager();
      if (SM.isWrittenInMainFile(CRD->getLocation())) {
        Dependencies.insert(CRD);
      } else {
        std::string File =
            llvm::sys::path::filename(SM.getFilename(CRD->getLocation())).str();
        HeaderIncludes.insert(File);
      }
    }
    return true;
  }

  std::set<const Decl*> getDependencies() const { return Dependencies; }
  std::set<std::string> getHeaderIncludes() const { return HeaderIncludes; }

 private:
  ASTContext* Context;
  std::set<const Decl*> Dependencies;
  std::set<std::string> HeaderIncludes;
};

class FunctionPrinter : public MatchFinder::MatchCallback {
 public:
  void run(const MatchFinder::MatchResult& Result) override
  {
    if (const FunctionDecl* FD =
            Result.Nodes.getNodeAs<FunctionDecl>("targetFunc")) {
      if (FD->hasBody()) {
        MyDependencyCollector Collector(Result.Context);
        Collector.TraverseDecl(const_cast<FunctionDecl*>(FD));

        std::string IncludesBuffer;
        std::string DepsBuffer;
        std::string FuncBuffer;

        llvm::raw_string_ostream IncludesStream(IncludesBuffer);
        llvm::raw_string_ostream DepsStream(DepsBuffer);
        llvm::raw_string_ostream FuncStream(FuncBuffer);

        // Collect includes
        for (const std::string& H : Collector.getHeaderIncludes()) {
          IncludesStream << "#include <" << H << ">\n";
        }

        // Collect dependencies
        for (const Decl* D : Collector.getDependencies()) {
          DepsStream << "\n=== Dependency ===\n";
          D->print(DepsStream);
        }

        // Collect the main function
        FuncStream << "\n=== Function ===\n";
        FD->print(FuncStream);

        // Flush all buffers
        IncludesStream.flush();
        DepsStream.flush();
        FuncStream.flush();

        // Print in the desired order
        llvm::outs() << IncludesBuffer;
        llvm::outs() << DepsBuffer;
        llvm::outs() << FuncBuffer;
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