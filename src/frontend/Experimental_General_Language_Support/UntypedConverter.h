#ifndef UNTYPED_CONVERTER_H
#define UNTYPED_CONVERTER_H

//-----------------------------------------------------------------------------------
// The SgUntyped::UntypedConverter class functions are used to convert
// SgUntypedNodes to regular SgNodes.
//-----------------------------------------------------------------------------------

namespace Untyped {

typedef std::vector<SgLocatedNode*> SgNodePtrList;

class UntypedConverter
  {
    public:

      UntypedConverter(bool isCaseInsensitive = false)
         {
            pCaseInsensitive = isCaseInsensitive;
         }

      void  setSourcePositionFrom      ( SgLocatedNode* toNode, SgLocatedNode* fromNode );
      void  setSourcePositionIncluding ( SgLocatedNode* toNode, SgLocatedNode* startNode, SgLocatedNode* endNode );

      void  setSourcePositionUnknown   ( SgLocatedNode* locatedNode );

      SgScopeStatement* initialize_global_scope ( SgSourceFile* file );

      void buildProcedureSupport (SgUntypedFunctionDeclaration* ut_function,
                                  SgProcedureHeaderStatement* procedureDeclaration, SgScopeStatement* scope);

      virtual bool
      convertLabel(SgUntypedStatement* ut_stmt, SgStatement* sg_stmt, SgScopeStatement* label_scope=NULL) = 0;

      virtual bool
      convertLabel(SgUntypedStatement* ut_stmt, SgStatement* sg_stmt,
                   SgLabelSymbol::label_type_enum label_type, SgScopeStatement* scope=NULL) = 0;

      virtual SgInitializedName*        convertSgUntypedInitializedName     (SgUntypedInitializedName*     ut_name,      SgType* sg_base_type);
      virtual SgInitializedNamePtrList* convertSgUntypedInitializedNameList (SgUntypedInitializedNameList* ut_name_list, SgType* sg_base_type);

      virtual void convertFunctionPrefix (SgUntypedExprListExpression* prefix_list, SgFunctionDeclaration* function_decl);

      virtual void setDeclarationModifiers (SgDeclarationStatement* decl, SgUntypedExprListExpression* mod_list);

      virtual SgType*   convertSgUntypedType        (SgUntypedType* ut_type,                SgScopeStatement* scope);
      virtual SgGlobal* convertSgUntypedGlobalScope (SgUntypedGlobalScope* ut_scope,        SgScopeStatement* scope);

      virtual SgModuleStatement*          convertSgUntypedModuleDeclaration        (SgUntypedModuleDeclaration*        ut_decl, SgScopeStatement* scope);
      virtual SgProgramHeaderStatement*   convertSgUntypedProgramHeaderDeclaration (SgUntypedProgramHeaderDeclaration* ut_decl, SgScopeStatement* scope);
      virtual SgProcedureHeaderStatement* convertSgUntypedSubroutineDeclaration    (SgUntypedSubroutineDeclaration*    ut_decl, SgScopeStatement* scope);
      virtual SgProcedureHeaderStatement* convertSgUntypedBlockDataDeclaration     (SgUntypedBlockDataDeclaration*     ut_decl, SgScopeStatement* scope);

      virtual SgProcedureHeaderStatement* convertSgUntypedFunctionDeclaration      (SgUntypedFunctionDeclaration*      ut_decl, SgScopeStatement* scope);
      virtual void                        convertSgUntypedFunctionDeclarationList  (SgUntypedFunctionDeclarationList*  ut_list, SgScopeStatement* scope);

   // Declaration statements
   //
      virtual SgDeclarationStatement* convertSgUntypedNameListDeclaration (SgUntypedNameListDeclaration* ut_decl, SgScopeStatement* scope);
      virtual SgImplicitStatement*    convertSgUntypedImplicitDeclaration (SgUntypedImplicitDeclaration* ut_decl, SgScopeStatement* scope);
      virtual SgVariableDeclaration*  convertSgUntypedVariableDeclaration (SgUntypedVariableDeclaration* ut_decl, SgScopeStatement* scope);
      virtual SgBasicBlock*           convertSgUntypedBlockStatement      (SgUntypedBlockStatement*      ut_stmt, SgScopeStatement* scope);

   // Executable statements
   //
      virtual SgExprStatement* convertSgUntypedAssignmentStatement (SgUntypedAssignmentStatement* ut_stmt, SgNodePtrList& children, SgScopeStatement* scope);
      virtual SgStatement*     convertSgUntypedExpressionStatement (SgUntypedExpressionStatement* ut_stmt, SgNodePtrList& children, SgScopeStatement* scope);
      virtual SgIfStmt*        convertSgUntypedIfStatement         (SgUntypedIfStatement*         ut_stmt, SgNodePtrList& children, SgScopeStatement* scope);
      virtual SgReturnStmt*    convertSgUntypedReturnStatement     (SgUntypedReturnStatement*     ut_stmt, SgNodePtrList& children, SgScopeStatement* scope);
      virtual SgStatement*     convertSgUntypedStopStatement       (SgUntypedStopStatement*       ut_stmt, SgNodePtrList& children, SgScopeStatement* scope);

      virtual SgStatement*     convertSgUntypedAbortStatement      (SgUntypedAbortStatement*  ut_stmt, SgScopeStatement* scope);
      virtual SgStatement*     convertSgUntypedExitStatement       (SgUntypedExitStatement*   ut_stmt, SgScopeStatement* scope);
      virtual SgStatement*     convertSgUntypedGotoStatement       (SgUntypedGotoStatement*   ut_stmt, SgScopeStatement* scope);
      virtual SgStatement*     convertSgUntypedLabelStatement_decl (SgUntypedLabelStatement*  ut_stmt, SgScopeStatement* scope);
      virtual SgStatement*     convertSgUntypedLabelStatement      (SgUntypedLabelStatement*  ut_stmt, SgStatement* stmt, SgScopeStatement* scope);
      virtual SgNullStatement* convertSgUntypedNullStatement       (SgUntypedNullStatement*   ut_stmt, SgScopeStatement* scope);
      virtual SgStatement*     convertSgUntypedOtherStatement      (SgUntypedOtherStatement*  ut_stmt, SgScopeStatement* scope);

   // Expressions
   //
      virtual SgExpression* convertSgUntypedExpression      (SgUntypedExpression* ut_expr);
      virtual SgExpression* convertSgUntypedExpression      (SgUntypedExpression* ut_expr, SgNodePtrList& children);
      virtual SgExpression* convertSgUntypedExpression      (SgUntypedExpression* ut_expr, SgNodePtrList& children, SgScopeStatement* scope);

      virtual SgValueExp*   convertSgUntypedValueExpression (SgUntypedValueExpression* ut_expr);
      virtual SgUnaryOp*    convertSgUntypedUnaryOperator   (SgUntypedUnaryOperator * untyped_operator, SgExpression* expr);
      virtual SgBinaryOp*   convertSgUntypedBinaryOperator  (SgUntypedBinaryOperator* untyped_operator, SgExpression* lhs, SgExpression* rhs);

      virtual SgExprListExp* convertSgUntypedExprListExpression  (SgUntypedExprListExpression * ut_expr_list);
      virtual SgExpression*  convertSgUntypedSubscriptExpression (SgUntypedSubscriptExpression* ut_expr);

    protected:

      bool pCaseInsensitive;
  };

} // namespace Untyped

// endif for UNTYPED_CONVERTER_H
#endif

