//
//  LHSComparators.cpp
//  Framework X
//
//  Created by Ruben Opdebeeck on 23/05/2017.
//  Copyright © 2017 Ruben Opdebeeck. All rights reserved.
//

#include "LHSComparators.hpp"

using namespace X;

//
// NOTE: The functions in this source file are possible quite inefficient and a lot of improvements can be made on their formatting.
//

// AST Node Kinds
#define ASTNODEKIND(NodeKind) static ASTNodeKind NodeKind##Class = ASTNodeKind::getFromNodeKind<NodeKind>()
ASTNODEKIND(Stmt);

#define COMPARE(NodeKind, body) \
ASTNODEKIND(NodeKind); \
static bool compare##NodeKind(const DynTypedNode &templNode, const DynTypedNode &potMatchNode, bool nameOnly) { \
const NodeKind* left(templNode.get<NodeKind>()); \
const NodeKind* right(potMatchNode.get<NodeKind>()); \
body \
}

#define EXTRA_CHECK(NodeKind) if (NodeKind##Class.isBaseOf(templNodeKind)) return compare##NodeKind(templNode, potMatchNode, nameOnly)

static bool compareTypes(const Type *left, const Type *right, bool nameOnly);

static bool compareQualTypes(const QualType &left, const QualType &right) {
    return left.getQualifiers() == right.getQualifiers() && compareTypes(left.getTypePtr(), right.getTypePtr(), false);
}

static bool compareTypes(const Type *left, const Type *right, bool nameOnly) {
    if (left->getTypeClass() != right->getTypeClass()) return false;
    
    switch (left->getTypeClass()) {
        case clang::Type::ConstantArray:
        case clang::Type::VariableArray:
        case clang::Type::DependentSizedArray:
        case clang::Type::IncompleteArray: {
            const auto *arrLeft = cast<ArrayType>(left);
            const auto *arrRight = cast<ArrayType>(right);
            return arrLeft->getIndexTypeQualifiers() == arrRight->getIndexTypeQualifiers()
                && arrLeft->getSizeModifier() == arrRight->getSizeModifier()
                && compareQualTypes(arrLeft->getElementType(), arrRight->getElementType());
        }
        case clang::Type::Builtin:
            return cast<BuiltinType>(left)->getKind() == cast<BuiltinType>(right)->getKind();
        case clang::Type::FunctionProto:
        case clang::Type::FunctionNoProto:
            return compareQualTypes(cast<FunctionType>(left)->getReturnType(), cast<FunctionType>(right)->getReturnType());
        case clang::Type::MemberPointer: {
            const auto *memberLeft = cast<MemberPointerType>(left);
            const auto *memberRight = cast<MemberPointerType>(right);
            return memberLeft->isMemberDataPointer() == memberRight->isMemberDataPointer()
                && compareQualTypes(memberLeft->getPointeeType(), memberRight->getPointeeType());
        }
        case clang::Type::Paren:
            return compareQualTypes(cast<ParenType>(left)->getInnerType(), cast<ParenType>(right)->getInnerType());
        case clang::Type::Pointer:
            return compareQualTypes(cast<PointerType>(left)->getPointeeType(), cast<PointerType>(right)->getPointeeType());
        case clang::Type::RValueReference:
        case clang::Type::LValueReference:
            return compareQualTypes(cast<ReferenceType>(left)->getPointeeType(), cast<ReferenceType>(right)->getPointeeType());
        case clang::Type::Enum:
        case clang::Type::Record:
            return nameOnly || cast<TagType>(left)->getDecl()->getNameAsString() == cast<TagType>(right)->getDecl()->getNameAsString();
        
        default:
            return true;
    }
}

COMPARE(TagDecl, {
    return left->getTagKind() == right->getTagKind();
})

COMPARE(TypeDecl, {
    if (!compareTypes(left->getTypeForDecl(), right->getTypeForDecl(), nameOnly)) return false;
    ASTNodeKind templNodeKind(templNode.getNodeKind());
    EXTRA_CHECK(TagDecl);
    
    return true;
})

COMPARE(CXXMethodDecl, {
    return left->isVirtual() == right->isVirtual() && left->isConst() == right->isConst()
    && left->isStatic() == right->isStatic();
})

COMPARE(ValueDecl, {
    if (!compareQualTypes(left->getType(), right->getType())) return false;
    
    ASTNodeKind templNodeKind(templNode.getNodeKind());
    EXTRA_CHECK(CXXMethodDecl);
    
    return true;
})

COMPARE(UsingDirectiveDecl, {
    return left->getNominatedNamespaceAsWritten()->getNameAsString() == right->getNominatedNamespaceAsWritten()->getNameAsString();
})

COMPARE(NamedDecl, {
    if (!nameOnly && left->getNameAsString() != right->getNameAsString()) return false;
    
    ASTNodeKind templNodeKind(templNode.getNodeKind());
    EXTRA_CHECK(TypeDecl);
    EXTRA_CHECK(ValueDecl);
    EXTRA_CHECK(UsingDirectiveDecl);
    
    return true;
})

COMPARE(Decl, {
    if (left->getKind() != right->getKind() || left->getAccess() != right->getAccess()) return false;
    
    ASTNodeKind templNodeKind(templNode.getNodeKind());
    EXTRA_CHECK(NamedDecl);
    
    return true;
})

COMPARE(BinaryOperator, {
    return left->getOpcode() == right->getOpcode();
})

COMPARE(CharacterLiteral, {
    return left->getKind() == right->getKind() && left->getValue() == right->getValue();
})

COMPARE(CXXBoolLiteralExpr, {
    return left->getValue() == right->getValue();
})

COMPARE(DeclRefExpr, {
    return compareDecl(DynTypedNode::create(*left->getDecl()), DynTypedNode::create(*right->getDecl()), nameOnly);
})

COMPARE(FloatingLiteral, {
    return left->isExact() == right->isExact() && left->getValue().bitwiseIsEqual(right->getValue());
})

COMPARE(IntegerLiteral, {
    return left->getValue() == right->getValue();
})

COMPARE(MemberExpr, {
    return left->isArrow() == right->isArrow() && compareDecl(DynTypedNode::create(*left->getMemberDecl()), DynTypedNode::create(*right->getMemberDecl()), nameOnly);
})

COMPARE(StringLiteral, {
    return left->getKind() == right->getKind() && left->getString() == right->getString();
})

COMPARE(UnaryOperator, {
    return left->getOpcode() == right->getOpcode();
})

bool X::compare(DynTypedNode templNode, DynTypedNode potMatchNode, bool nameOnly) {
    ASTNodeKind templNodeKind(templNode.getNodeKind());
    
    // At least the node kind must be the same.
    if (!templNodeKind.isSame(potMatchNode.getNodeKind()) && !templNodeKind.isNone() && !potMatchNode.getNodeKind().isNone()) return false;
    
    // Further checks are needed for certain node types
    if (StmtClass.isBaseOf(templNodeKind)) {
        EXTRA_CHECK(BinaryOperator);
        EXTRA_CHECK(CharacterLiteral);
        EXTRA_CHECK(CXXBoolLiteralExpr);
        EXTRA_CHECK(DeclRefExpr);
        EXTRA_CHECK(FloatingLiteral);
        EXTRA_CHECK(IntegerLiteral);
        EXTRA_CHECK(MemberExpr);
        EXTRA_CHECK(StringLiteral);
        EXTRA_CHECK(UnaryOperator);
    } else EXTRA_CHECK(Decl);
    
    return true;
}
