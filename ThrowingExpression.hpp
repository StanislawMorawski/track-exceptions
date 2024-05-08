#pragma once
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include <string>


struct TrackedExceptionType {
  clang::Type const *type;
  clang::SourceRange range;
  bool isExplicit;

  TrackedExceptionType(clang::Type const *type, clang::SourceRange range, bool isExplicit)
      : type(type), range(range), isExplicit(isExplicit) {}

  bool operator==(TrackedExceptionType const &other) const {
    return type->getUnqualifiedDesugaredType() ==
           other.type->getUnqualifiedDesugaredType();
  }

  bool rangeLess(TrackedExceptionType const &other) const {
    return range.getBegin() < other.range.getBegin() ||
           (range.getBegin() == other.range.getBegin() &&
            range.getEnd() < other.range.getEnd());
  }

  bool operator<(TrackedExceptionType const &other) const {
    return type->getUnqualifiedDesugaredType() <
           other.type->getUnqualifiedDesugaredType();
  }

  std::string toString() const {
    if (auto *decl = type->getAsCXXRecordDecl(); decl) {
      return decl->getNameAsString();
    }
    if (auto *decl = type->getPointeeCXXRecordDecl(); decl) {
      return decl->getNameAsString();
    }
    return static_cast<clang::QualType>(type->getCanonicalTypeUnqualified())
        .getAsString();
  }
};