#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

static llvm::cl::OptionCategory ToolCategory("extract-func options");

class ExtractorCallback : public MatchFinder::MatchCallback {
 public:
  ExtractorCallback(Rewriter& R) : TheRewriter(R) {}

  void run(const MatchFinder::MatchResult& Result) override
  {
    if (const FunctionDecl* FD =
            Result.Nodes.getNodeAs<FunctionDecl>("targetFunc")) {
      if (FD->hasBody()) {
        SourceManager& SM = *Result.SourceManager;
        const LangOptions& LangOpts = Result.Context->getLangOpts();

        SourceRange Range = FD->getSourceRange();
        CharSourceRange CharRange = CharSourceRange::getTokenRange(Range);
        std::string FuncText =
            Lexer::getSourceText(CharRange, SM, LangOpts).str();

        llvm::errs() << "Extracting:\n" << FuncText << "\n";

        std::error_code EC;
        llvm::raw_fd_ostream Out("extracted.cpp", EC, llvm::sys::fs::OF_Text);
        if (EC) {
          llvm::errs() << "Could not open output file: " << EC.message()
                       << "\n";
          return;
        }

        Out << FuncText << "\n";
        Out.close();
      }
    }
  }

 private:
  Rewriter& TheRewriter;
};

class ExtractorAction : public ASTFrontendAction {
 public:
  void EndSourceFileAction() override {}

  std::unique_ptr<ASTConsumer> CreateASTConsumer(
      CompilerInstance& CI, StringRef InFile) override
  {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    Finder.addMatcher(
        functionDecl(hasName("my_kernel")).bind("targetFunc"),
        new ExtractorCallback(TheRewriter));
    return Finder.newASTConsumer();
  }

 private:
  Rewriter TheRewriter;
  MatchFinder Finder;
};

int
main(int argc, const char** argv)
{
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, ToolCategory);
  if (!ExpectedParser) {
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(
      OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

  return Tool.run(newFrontendActionFactory<ExtractorAction>().get());
}
