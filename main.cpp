#include "ExceptionsTracker.hpp"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/Support/raw_ostream.h"

#include "clang-tidy/ClangTidy.h"
#include <memory>


using namespace clang::tooling;
using namespace llvm;

using namespace clang;
using namespace clang::ast_matchers;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("track-exceptions options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser &OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  clang::tidy::ClangTidyOptions DefaultOptions;
  DefaultOptions.Checks =  {"ExceptionsTracker"};;

  auto Options = std::make_unique<clang::tidy::FileOptionsProvider>(
      clang::tidy::ClangTidyGlobalOptions{}, DefaultOptions, clang::tidy::ClangTidyOptions{});
  auto Context = std::make_unique<clang::tidy::ClangTidyContext>(std::move(Options));


  clang::tidy::ClangTidyDiagnosticConsumer DiagConsumer(*Context, nullptr, true, false);
  DiagnosticsEngine DE(new DiagnosticIDs(), new DiagnosticOptions(),
                       &DiagConsumer, /*ShouldOwnClient=*/false);
  Context->setDiagnosticsEngine(&DE);

  ExceptionsTracker exceptionsTracker{"ExceptionsTracker", Context.get()};
  
  MatchFinder finder;
  exceptionsTracker.registerMatchers(&finder);
  Tool.setDiagnosticConsumer(&DiagConsumer);
  Tool.run(newFrontendActionFactory(&finder).get());
  auto Errors = DiagConsumer.take();

  unsigned WErrorCount = 0;

  llvm::IntrusiveRefCntPtr<vfs::OverlayFileSystem> BaseFS(
      new vfs::OverlayFileSystem(vfs::getRealFileSystem()));

  clang::tidy::handleErrors(Errors, *Context, clang::tidy::FB_NoFix,
               WErrorCount, BaseFS);

  if(Errors.empty()) {
    llvm::errs() << "No errors found :)\n";
  }

  if (WErrorCount) {
      StringRef Plural = WErrorCount == 1 ? "" : "s";
      llvm::errs() << WErrorCount << " warning" << Plural << " treated as error"
                   << Plural << "\n";
    return 1;
  }

  bool FoundErrors = llvm::any_of(Errors, [](const clang::tidy::ClangTidyError &E) {
    return E.DiagLevel == clang::tidy::ClangTidyError::Error;
  });
  if (FoundErrors) {
    // TODO: Figure out when zero exit code should be used with -fix-errors:
    //   a. when a fix has been applied for an error
    //   b. when a fix has been applied for all errors
    //   c. some other condition.
    // For now always returning zero when -fix-errors is used.
    if (false)
      return 0;
    llvm::errs() << "Found compiler error(s).\n";
    return 1;
  }

  return 0;
}