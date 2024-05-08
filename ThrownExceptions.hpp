#pragma once
#include "ThrowingExpression.hpp"
#include "defines.hpp"

#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include <set>

inline bool isSameOrDerrived(const clang::Type *type, const clang::Type *other) {
  if (type == other)
    return true;
  auto *lhsDecl = type->getAsCXXRecordDecl();
  auto *rhsDecl = other->getAsCXXRecordDecl();
  if (!lhsDecl || !rhsDecl)
    return false;
  return lhsDecl->isDerivedFrom(rhsDecl);
}

struct ThrownExceptions {
  std::set<TrackedExceptionType> exceptions;

  void try_insert(TrackedExceptionType exp,
                  std::vector<const clang::Type *> caughtTypes) {
    for (auto *caughtType : caughtTypes) {
      if (isSameOrDerrived(exp.type, caughtType)) {
        return;
      }
    }
    exceptions.insert(std::move(exp));
  }

  void add(clang::Decl const *f, clang::SourceRange range,
           std::vector<const clang::Type *> caughtTypes) {
    for (const auto *attr : f->specific_attrs<clang::AnnotateAttr>()) {
      if (attr->getAnnotation() == ThrowAnnotation) {
        for (auto *arg : attr->args()) {
          if (arg) {
            auto exception = TrackedExceptionType{
                arg->getType()->getPointeeType().getTypePtr(), range, true};

            try_insert(exception, caughtTypes);
          }
        }
      }
    }
  }
};