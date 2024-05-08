#include "ExceptionsTracker.hpp"
#include "defines.hpp"
#include "ThrownExceptions.hpp"
#include "AnnotatedExceptions.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/DiagnosticIDs.h"
#include <clang/AST/StmtCXX.h>
#include <sstream>

using namespace clang::ast_matchers;
using namespace clang;


namespace {
bool rethrowsException(Stmt *node) {
  if (auto *throwExpr = dyn_cast<CXXThrowExpr>(node); throwExpr) {
    auto subexpr = throwExpr->getSubExpr();
    if(!subexpr) return true;
  }

  for (auto *kid : node->children()) {
    if (kid) {
      if (auto *catchStmt = dyn_cast<CXXCatchStmt>(node); catchStmt) continue;
      if(rethrowsException(kid)) return true;
    }
  }
  return false;
}

void getThrownTypes(
    Stmt *node, ThrownExceptions &exceptions, const SourceManager &soMan,
    std::vector<const Type *> caughtTypes = {}) { // [[gnu::nonnull]]
  if (auto *tryStmt = dyn_cast<CXXTryStmt>(node); tryStmt) {
    for (unsigned i = 0; i < tryStmt->getNumHandlers(); ++i) {
      auto *handler = tryStmt->getHandler(i);
      getThrownTypes(handler, exceptions, soMan, caughtTypes);
    }
    for (unsigned i = 0; i < tryStmt->getNumHandlers(); ++i) {
      auto *handler = tryStmt->getHandler(i);
      if (handler->getExceptionDecl() == nullptr) {
        return;
      }
      auto caughtType = handler->getCaughtType();

      if(!rethrowsException(node)) {
        caughtTypes.push_back(caughtType.getTypePtr());
      }
    }
    getThrownTypes(tryStmt->getTryBlock(), exceptions, soMan, caughtTypes);
    return;
  }

  auto range = node->getSourceRange();
  if (auto *throwExpr = dyn_cast<CXXThrowExpr>(node); throwExpr) {
    auto subexpr = throwExpr->getSubExpr();
    if(!subexpr) {
      // rethrowing exception
      return;
    }
    auto const *type = subexpr->getType().getTypePtr();
    auto const isTypeExplicit = [&]() {
      auto *TypeRecord = type->getAsCXXRecordDecl();
      if (TypeRecord) {
        for (const auto *typeAttr :
             TypeRecord->specific_attrs<AnnotateAttr>()) {
          if (typeAttr->getAnnotation() == ExplicitErrorAnnotation) {
            return true;
          }
        }
      }
      return false;
    }();
    exceptions.try_insert({type, range, isTypeExplicit}, caughtTypes);
  } else if (auto *constructor = dyn_cast<CXXConstructExpr>(node)) {
    exceptions.add(constructor->getConstructor(), range, caughtTypes);
  } else if (auto *expr = dyn_cast<Expr>(node); expr) {
    if (auto *decl = expr->getReferencedDeclOfCallee(); decl) {
      exceptions.add(decl, range, caughtTypes);
    }
  }

  for (auto *kid : node->children()) {
    if (kid) {
      getThrownTypes(kid, exceptions, soMan, caughtTypes);
    }
  }
}

std::pair<std::vector<TrackedExceptionType>, std::vector<TrackedExceptionType>>
getExceptionsMissmatch(std::set<TrackedExceptionType> &expectedExceptions,
                    std::set<TrackedExceptionType> &actualExceptions) {
  std::vector<TrackedExceptionType> PrevRecordsNotFoundInNew;
  std::set_difference(actualExceptions.begin(), actualExceptions.end(),
                      expectedExceptions.begin(), expectedExceptions.end(),
                      std::back_inserter(PrevRecordsNotFoundInNew));
  std::vector<TrackedExceptionType> NewRecordsNotFoundInPrev;
  std::set_difference(expectedExceptions.begin(), expectedExceptions.end(),
                      actualExceptions.begin(), actualExceptions.end(),
                      std::back_inserter(NewRecordsNotFoundInPrev));
  return {std::move(PrevRecordsNotFoundInNew),
          std::move(NewRecordsNotFoundInPrev)};
}
} // namespace

void ExceptionsTracker::registerMatchers(MatchFinder *Finder) {
  auto functionMatcher = functionDecl().bind("functionMatch");
  auto classMatcher = cxxRecordDecl().bind("classMatch");
  Finder->addMatcher(functionMatcher, this);
  Finder->addMatcher(classMatcher, this);
}

void ExceptionsTracker::checkFunction(FunctionDecl const *f, SourceManager& soMan) {
  AnnotatedExceptions annotatedExceptions;
  annotatedExceptions.add(f);
  for(auto *prevDecl = f->getPreviousDecl(); prevDecl; prevDecl = prevDecl->getPreviousDecl()) {
    AnnotatedExceptions PrevAnnotatedExceptions;
    PrevAnnotatedExceptions.add(prevDecl);
    auto [unannotatedexceptions, notThrownExceptions] = getExceptionsMissmatch(PrevAnnotatedExceptions.exceptions, annotatedExceptions.exceptions);
    if (!unannotatedexceptions.empty() || !notThrownExceptions.empty()) {
      diag(f->getLocation(), "Missmatch between throw annotations in "
                             "function %0 and declarations")
          << f;
      // << FixItHint::CreateInsertion(prevDecl->getLocation(),
      // "[[THROWS()]]");
      diag(prevDecl->getLocation(),
           "Previous declaration:", DiagnosticIDs::Note);

      printUnannotatedExceptions(unannotatedexceptions);
      printExtraExceptionAnnotations(notThrownExceptions);
      return;
    }
  }
  ThrownExceptions thrownExceptions;
  if (f->hasBody() && !f->isDefaulted() && f->doesThisDeclarationHaveABody()) {
    if(auto *constructor = dyn_cast<CXXConstructorDecl>(f); constructor) {
      for(auto* init : constructor->inits()) {
        getThrownTypes(init->getInit(), thrownExceptions, soMan);
      }
    }
    getThrownTypes(f->getBody(), thrownExceptions, soMan);
    auto [unannotatedExceptions, notThrownExceptions] =
        getExceptionsMissmatch(annotatedExceptions.exceptions, thrownExceptions.exceptions);
    llvm::erase_if(unannotatedExceptions,
                   [](TrackedExceptionType const &x) { return !x.isExplicit; });
    if (!unannotatedExceptions.empty() || !notThrownExceptions.empty()) {
      diag(f->getLocation(), "Missmatch between throw annotation and "
                             "actually thrown exceptions in function %0")
          << f;

      printUnannotatedExceptions(unannotatedExceptions);
      printExtraExceptionAnnotations(notThrownExceptions);
    }
  }
}

void ExceptionsTracker::checkClass(CXXRecordDecl const *cls, SourceManager& soMan) {
  if(!cls->hasDefinition()) return;
  auto defaultFilter = [](CXXConstructorDecl const *decl) {
    return decl->isDefaultConstructor();
  };
  auto moveFilter = [](CXXConstructorDecl const *decl) {
    return decl->isMoveConstructor();
  };
  auto copyFilter = [](CXXConstructorDecl const *decl) {
    return decl->isCopyConstructor();
  };

  auto evaluateSingleConstructor = [&](auto &filter, const char* name) { 
    AnnotatedExceptions declaredExceptions;
    AnnotatedExceptions fieldExceptions;

    auto loc = cls->getLocation();

    for(auto *ctor : cls->ctors()) {
      if(filter(ctor)) {
        if(! ctor->isDefaulted()) return;
        declaredExceptions.add(ctor);
        loc = ctor->getLocation();
      }
    }

    for(auto *field : cls->fields()) {
      auto type = field->getType();
      if(type->isPointerType() || type->isReferenceType()) continue;
      auto *recordDecl = type->getAsCXXRecordDecl();
      if(recordDecl) {
        for(auto *ctor : recordDecl->ctors()) {
          if(filter(ctor)) {
            fieldExceptions.add(ctor);
          }
        }
      }
    }

    auto [unannotatedExceptions, notThrownExceptions] = getExceptionsMissmatch(declaredExceptions.exceptions, fieldExceptions.exceptions);

    
    if (!unannotatedExceptions.empty() || !notThrownExceptions.empty()) {
      diag(loc, "Missmatch between throw annotation and "
                             "actually thrown exceptions in %0 constructor of class %1")
          << name << cls;

      printUnannotatedExceptions(unannotatedExceptions);
      printExtraExceptionAnnotations(notThrownExceptions);
      return;
    }
    
  };

    if(cls->hasDefaultConstructor() && !cls->hasUserProvidedDefaultConstructor()) {
      evaluateSingleConstructor(defaultFilter, "default");
    }
    if(cls->hasNonTrivialMoveConstructor()) {
      evaluateSingleConstructor(moveFilter, "move");
    }
    if(cls->hasNonTrivialCopyConstructor()) {
      evaluateSingleConstructor(copyFilter, "copy");
    }

}

void ExceptionsTracker::check(const MatchFinder::MatchResult &Result) {
  auto& soMan = *Result.SourceManager;
  context_hack->setSourceManager(&soMan);
  context_hack->setASTContext(Result.Context);
  if (const FunctionDecl *f =
          Result.Nodes.getNodeAs<clang::FunctionDecl>("functionMatch")) {
    checkFunction(f, soMan);
  } else if (const CXXRecordDecl *cls =
                 Result.Nodes.getNodeAs<clang::CXXRecordDecl>("classMatch")) {
    checkClass(cls, soMan);
  }
}

void ExceptionsTracker::printUnannotatedExceptions(
    std::vector<TrackedExceptionType> const &exceptions) {
  for (auto const &rec : exceptions) {
    std::stringstream ss;
    ss << "missing exception annotation '" << rec.toString() << "' thrown at:";
    diag(rec.range.getBegin(), ss.str(), DiagnosticIDs::Note);
  }
}

void ExceptionsTracker::printExtraExceptionAnnotations(
    std::vector<TrackedExceptionType> const &exceptions) {
  for (auto const &rec : exceptions) {
    std::stringstream ss;
    ss << "extra exception annotation '" << rec.toString() << "' annotated at:";
    diag(rec.range.getBegin(), ss.str(), DiagnosticIDs::Note);
  }
}
