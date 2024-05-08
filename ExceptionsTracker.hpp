#pragma once
#include "ThrowingExpression.hpp"

#include "clang/Basic/SourceManager.h"
#include "clang-tidy/ClangTidyCheck.h"

class ExceptionsTracker : public clang::tidy::ClangTidyCheck {
public:
  ExceptionsTracker(clang::StringRef Name, clang::tidy::ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context), context_hack(Context) {}
  void registerMatchers(clang::ast_matchers::MatchFinder *Finder) override;
  void check(const clang::ast_matchers::MatchFinder::MatchResult &Result) override;
  void checkFunction(clang::FunctionDecl const *f, clang::SourceManager& soMan);
  void checkClass(clang::CXXRecordDecl const *f, clang::SourceManager& soMan);
  private:
  

  void printUnannotatedExceptions(std::vector<TrackedExceptionType> const &exceptions);
  void printExtraExceptionAnnotations(std::vector<TrackedExceptionType> const &exceptions);
  private:
  clang::tidy::ClangTidyContext *context_hack;
};
