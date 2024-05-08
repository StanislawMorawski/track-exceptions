#pragma once
#include "ThrowingExpression.hpp"
#include "defines.hpp"

#include "clang/AST/Type.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include <set>



struct AnnotatedExceptions {
  std::set<TrackedExceptionType> exceptions;

  void add(clang::Decl const *f) {
    for (const auto *attr : f->specific_attrs<clang::AnnotateAttr>()) {
      if (attr->getAnnotation() == ThrowAnnotation) {
        for (auto *arg : attr->args()) {
          if (arg) {
            auto type = arg->getType()->getPointeeType();
            if (type.isNull()) {
              continue;
            }
            auto exception = TrackedExceptionType{type.getTypePtr(),
                                                arg->getSourceRange(), true};

            exceptions.insert(exception);
          }
        }
      }
    }
  }
};