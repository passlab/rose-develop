#ifndef ATERM_TO_UNTYPED_FORTRAN_TRAVERSAL_H
#define ATERM_TO_UNTYPED_FORTRAN_TRAVERSAL_H

#include "ATerm/ATermToUntypedTraversal.h"
#include "FASTNodes.h"

namespace ATermSupport {

class ATermToUntypedFortranTraversal : public ATermToUntypedTraversal
{
 public:

   ATermToUntypedFortranTraversal(SgSourceFile* source);

//R201
ATbool traverse_Program(ATerm term);

ATbool traverse_ProgramUnitList(ATerm term, SgUntypedGlobalScope* scope);

ATbool traverse_InitialSpecPart(ATerm term, SgUntypedDeclarationStatementList* decl_list);
ATbool traverse_SpecAndExecPart(ATerm term, SgUntypedDeclarationStatementList* decl_list,
                                                       SgUntypedStatementList* stmt_list);
// R204
ATbool traverse_SpecificationPart(ATerm term, SgUntypedDeclarationStatementList* decl_list);

// R205
ATbool traverse_OptImplicitPart(ATerm term, SgUntypedDeclarationStatementList* decl_list);

// R206
ATbool traverse_ImplicitPartStmtList(ATerm term, SgUntypedDeclarationStatementList* decl_list);

// R207
ATbool traverse_DeclarationConstruct     (ATerm term, SgUntypedDeclarationStatementList* decl_list);
ATbool traverse_DeclarationConstructList (ATerm term, SgUntypedDeclarationStatementList* decl_list);

// R209
ATbool traverse_ExecutionPartConstruct      (ATerm term, SgUntypedStatementList* stmt_list);
ATbool traverse_BlockExecutionPartConstruct (ATerm term, SgUntypedStatementList* stmt_list);

ATbool traverse_OptInternalSubprogramPart (ATerm term, SgUntypedOtherStatement** contains_stmt,
                                                       SgUntypedScope* scope);
// R211
ATbool traverse_InternalSubprogramList    (ATerm term, SgUntypedScope* scope);

ATbool traverse_SpecStmt( ATerm term, SgUntypedDeclarationStatementList* decl_list );
ATbool traverse_ExecStmt( ATerm term,            SgUntypedStatementList* stmt_list );

ATbool traverse_Name       ( ATerm term, std::string & name  );
ATbool traverse_NameString ( ATerm term, std::string & name  );
ATbool traverse_OptName    ( ATerm term, std::string & name  );
ATbool traverse_OptLabel   ( ATerm term, std::string & label );
ATbool traverse_eos        ( ATerm term, std::string & eos   );

ATbool traverse_NameList   ( ATerm term, SgUntypedNameList* name_list );

ATbool traverse_StartCommentBlock(ATerm term, std::string & start_comments);
ATbool traverse_OptModuleNature(ATerm term, FAST::UseStmt::ModuleNature* var_OptModuleNature);

// R305
ATbool traverse_LiteralConstant(ATerm term, SgUntypedExpression** var_expr);

// R309
ATbool traverse_Operator(ATerm term, SgUntypedExpression** var_expr);

// R310
ATbool traverse_DefinedOperator(ATerm term, std::string & name);

// R405
ATbool traverse_KindSelector(ATerm term, SgUntypedExpression** expr);

// R422
ATbool traverse_OptCharLength(ATerm term, SgUntypedExpression** expr);

// R501
ATbool traverse_TypeDeclarationStmt(ATerm term, SgUntypedDeclarationStatementList* decl_list);

// R502
ATbool traverse_OptAttrSpecList(ATerm term, SgUntypedExprListExpression* attr_list);
ATbool traverse_CudaAttributesSpec(ATerm term, SgUntypedOtherExpression** attr_spec);

// R503
ATbool traverse_EntityDecl     (ATerm term, SgUntypedType* declared_type, SgUntypedInitializedNameList* name_list);
ATbool traverse_EntityDeclList (ATerm term, SgUntypedType* declared_type, SgUntypedInitializedNameList* name_list);

// R505
ATbool traverse_OptInitialization(ATerm term, SgUntypedExpression** expr);

// R509
ATbool traverse_CoarraySpec    (ATerm term, SgUntypedType* base_type, SgUntypedArrayType** array_type);
ATbool traverse_OptCoarraySpec (ATerm term, SgUntypedType* base_type, SgUntypedArrayType** array_type);

// R515
ATbool traverse_ArraySpec      (ATerm term, SgUntypedType* base_type, SgUntypedArrayType** array_type);
ATbool traverse_OptArraySpec   (ATerm term, SgUntypedType* base_type, SgUntypedArrayType** array_type);

// R516
ATbool traverse_ExplicitShape    (ATerm term, SgUntypedExpression** lower_bound, SgUntypedExpression** upper_bound);
ATbool traverse_ExplicitShapeList(ATerm term, SgUntypedExprListExpression* dim_info);

// R519
ATbool traverse_AssumedShape   (ATerm term, SgUntypedExpression** lower_bound);

// R521
ATbool traverse_AssumedSize    (ATerm term, SgUntypedType* declared_type, SgUntypedArrayType** array_type);

// R522
ATbool traverse_AssumedOrImpliedSpec(ATerm term, SgUntypedExpression** lower_bound);

// R560
ATbool traverse_ImplicitStmt(ATerm term, SgUntypedDeclarationStatementList* decl_list);
ATbool traverse_ImplicitSpecList(ATerm term, std::vector<FAST::ImplicitSpec> & ref_ImplicitSpecList);

// R611
ATbool traverse_DataRef(ATerm term, SgUntypedExpression** var_expr);
// R612
ATbool traverse_PartRef(ATerm term, SgUntypedExpression** var_expr);

// R620
ATbool traverse_OptSectionSubscripts(ATerm term);
// R624
ATbool traverse_OptImageSelector(ATerm term);

ATbool traverse_ContinueStmt(ATerm term, SgUntypedStatement** continue_stmt);

ATbool traverse_DeclarationTypeSpec ( ATerm term, SgUntypedType** type );
ATbool traverse_IntrinsicTypeSpec   ( ATerm term, SgUntypedType** type );
//TODO traverse_DerivedTypeSpec

// R722
ATbool traverse_Expression ( ATerm term, SgUntypedExpression** expr );
ATbool traverse_OptExpr    ( ATerm term, SgUntypedExpression** expr );

// R732
ATbool traverse_AssignmentStmt(ATerm term, SgUntypedStatementList* stmt_list);

// R801
ATbool traverse_Block(ATerm term, SgUntypedBlockStatement** block_list);

// R832
ATbool traverse_IfConstruct(ATerm term, SgUntypedStatementList* stmt_list);

// R833
ATbool traverse_IfThenStmt(ATerm term, SgUntypedExpressionStatement** if_then_stmt);

// R834
ATbool traverse_ElseIfStmt      (ATerm term, SgUntypedExpressionStatement** else_if_stmt);
ATbool traverse_ElseIfStmtList  (ATerm term, SgUntypedIfStatement** if_stmt, SgUntypedIfStatement** last_if_stmt);
ATbool traverse_ElseIfStmtBlock (ATerm term, SgUntypedIfStatement** if_stmt);

// R835
ATbool traverse_ElseStmt(ATerm term, SgUntypedStatement** else_stmt);
ATbool traverse_OptElseStmtAndBlock(ATerm term, SgUntypedStatement** else_stmt, SgUntypedBlockStatement** false_body);

// R851
ATbool traverse_GotoStmt(ATerm term, SgUntypedStatementList* stmt_list);

// R854
ATbool traverse_ContinueStmt(ATerm term, SgUntypedStatementList* stmt_list);

// R855
ATbool traverse_StopStmt      ( ATerm term, SgUntypedStatementList* stmt_list  );
ATbool traverse_ErrorStopStmt ( ATerm term, SgUntypedStatementList* stmt_list  );
ATbool traverse_OptStopCode   ( ATerm term, SgUntypedExpression** var_StopCode );

// R1101
ATbool traverse_MainProgram(ATerm term, SgUntypedScope* scope);
ATbool traverse_OptProgramStmt(ATerm term, SgUntypedNamedStatement** program_stmt);
ATbool traverse_EndProgramStmt(ATerm term, SgUntypedNamedStatement** var_EndProgramStmt);

// R1104
ATbool traverse_Module    (ATerm term, SgUntypedScope* scope);

// R1105
ATbool traverse_ModuleStmt(ATerm term, SgUntypedNamedStatement** module_stmt);

// R1106
ATbool traverse_EndModuleStmt(ATerm term, SgUntypedNamedStatement** end_module_stmt);

// R1107
ATbool traverse_OptModuleSubprogramPart(ATerm term, SgUntypedOtherStatement** contains_stmt, SgUntypedScope* scope);

// R1108
ATbool traverse_ModuleSubprogramList(ATerm term, SgUntypedScope* scope);

// R1109
ATbool traverse_UseStmt(ATerm term, SgUntypedDeclarationStatementList* decl_list);
ATbool traverse_UseStmtList(ATerm term, SgUntypedDeclarationStatementList* decl_list);

// R1110
ATbool traverse_OptModuleNature(ATerm term, SgToken::ROSE_Fortran_Keywords & module_nature);

// R1111, R1112
ATbool traverse_RenameOrOnlyList(ATerm term, bool isOnlyList, SgUntypedTokenPairList* rename_or_only_list);

// R1116
ATbool traverse_Submodule (ATerm term, SgUntypedScope* scope);

// R1117
ATbool traverse_SubmoduleStmt(ATerm term, SgUntypedNamedStatement** submodule_stmt,
                              std::string & ancestor, std::string & parent);
// R1118
ATbool traverse_ParentIdentifier(ATerm term, std::string & ancestor, std::string & parent);

// R1119
ATbool traverse_EndSubmoduleStmt(ATerm term, SgUntypedNamedStatement** end_submodule_stmt);

// R1120
ATbool traverse_BlockData (ATerm term, SgUntypedScope* scope);

// R1121
ATbool traverse_BlockDataStmt(ATerm term, SgUntypedNamedStatement** block_data_stmt);

// R1122
ATbool traverse_EndBlockDataStmt(ATerm term, SgUntypedNamedStatement** end_block_data_stmt);

// R1201
ATbool traverse_InterfaceBlock(ATerm term, SgUntypedDeclarationStatementList* parent_decl_list);

// R1202
ATbool traverse_InterfaceSpecificationList(ATerm term, SgUntypedFunctionDeclarationList* func_list);

// R1203
ATbool traverse_InterfaceStmt(ATerm term, SgUntypedInterfaceDeclaration** interface_decl);

// R1204
ATbool traverse_EndInterfaceStmt(ATerm term, SgUntypedNamedStatement** end_interface_stmt);

// R1205
ATbool traverse_InterfaceBody(ATerm term, SgUntypedFunctionDeclarationList* func_list);

// R1206
ATbool traverse_ProcedureStmt(ATerm term, SgUntypedFunctionDeclarationList* func_list);

// R1207
ATbool traverse_OptGenericSpec(ATerm term, std::string & name, SgUntypedToken** generic_spec);

// R1209
ATbool traverse_ImportStmt(ATerm term, SgUntypedDeclarationStatementList* decl_list);
ATbool traverse_ImportStmtList(ATerm term, SgUntypedDeclarationStatementList* decl_list);

// R1210
ATbool traverse_ExternalStmt(ATerm term, SgUntypedDeclarationStatementList* decl_list);

// R1225
ATbool traverse_OptPrefix(ATerm term, SgUntypedExprListExpression* prefix_list, SgUntypedType** type);
ATbool traverse_CudaAttributesPrefix(ATerm term, SgUntypedOtherExpression** prefix);

// R1227
ATbool traverse_FunctionSubprogram(ATerm term, SgUntypedScope* scope);

// R1229
ATbool traverse_OptProcLanguageBindingSpec(ATerm term);

// R1231
ATbool traverse_OptSuffix(ATerm term);

// R1232
ATbool traverse_EndFunctionStmt(ATerm term, SgUntypedNamedStatement** end_function_stmt);

// R1233
ATbool traverse_SubroutineSubprogram(ATerm term, SgUntypedScope* scope);

// R1235
ATbool traverse_OptDummyArgList(ATerm term, SgUntypedInitializedNameList* param_list);

// R1236
ATbool traverse_EndSubroutineStmt(ATerm term, SgUntypedNamedStatement** end_subroutine_stmt);

// R1237
ATbool traverse_SeparateModuleSubprogram(ATerm term, SgUntypedScope* scope);

// R1238
ATbool traverse_MpSubprogramStmt    ( ATerm term, SgUntypedNamedStatement**     mp_subprogram_stmt );
ATbool traverse_EndMpSubprogramStmt ( ATerm term, SgUntypedNamedStatement** end_mp_subprogram_stmt );

// R1240 entry-stmt

// R1141
ATbool traverse_ReturnStmt   ( ATerm term, SgUntypedStatementList* stmt_list );

// R1142
ATbool traverse_ContainsStmt ( ATerm term, SgUntypedOtherStatement** contains_stmt );

ATbool traverse_CudaAttributesStmt(ATerm term, SgUntypedDeclarationStatementList* decl_list);

}; // class Traversal
}  // namespace ATermSupport

#endif
