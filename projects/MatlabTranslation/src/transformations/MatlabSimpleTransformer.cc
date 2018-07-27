#include <map>
#include <algorithm>
#include <iostream>
#include <string>
#include <boost/foreach.hpp>

#include "MatlabSimpleTransformer.h"

#include "rose.h"
#include "sageGeneric.h"

#include "typeInference/FastNumericsRoseSupport.h"
#include "utility/utils.h"
#include "utility/FunctionReturnAttribute.h"


// #include "ast/SgColonExpression.h"

namespace sb = SageBuilder;
namespace si = SageInterface;
namespace fn = FastNumericsRoseSupport;
namespace ru = RoseUtils;

  template <class SageNode>
  struct TransformExecutor
  {
    typedef void (*TransformExecutorFn)(SageNode*);

    explicit
    TransformExecutor(TransformExecutorFn fun)
    : fn(fun)
    {}

    void handle(SgNode& n)   { std::cerr << "Unkown type: " << typeid(n).name() << std::endl; ROSE_ASSERT(false); }
    void handle(SageNode& n) { fn(&n); }

    TransformExecutorFn fn;
  };

  template <class SageNode>
  static inline
  TransformExecutor<SageNode>
  createTransformExecutor(void (* fn)(SageNode*))
  {
    return TransformExecutor<SageNode>(fn);
  }

  template <class TFn>
  static
  void forAllNodes(SgProject* proj, TFn fn, VariantT v_sageNode)
  {
    Rose_STL_Container<SgNode*> nodes = NodeQuery::querySubTree(proj, v_sageNode);

    for(Rose_STL_Container<SgNode*>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      sg::dispatch(createTransformExecutor(fn), *it);
    }
  }

namespace MatlabToCpp
{
  //
  // ForloopTransformer

  static
  void tfForLoop(SgMatlabForStatement* matlabFor)
  {
    SgExpression*     index = matlabFor->get_index();
    SgScopeStatement* body = isSgScopeStatement(matlabFor->get_body());
    ROSE_ASSERT(body);

    SgExpression*     range = matlabFor->get_range();
    SgExpression*     init = NULL;
    SgExpression*     increment = NULL;
    SgStatement*      test = NULL;

    //The RHS in i = rangeExp could be a real range or other expressions return a range.
    if (SgRangeExp* rangeExp = isSgRangeExp(range))
    {
      //If the expression is a real range, we can just convert it to a normal for loop

      SgExpression *start = rangeExp->get_start();
      SgExpression *stride = rangeExp->get_stride();

      if(stride == NULL)
      {
        stride = sb::buildIntVal(1);
      }

      SgExpression *end = rangeExp->get_end();

      init = start;//sb::buildAssignOp(index, start);
      test = sb::buildExprStatement(sb::buildLessOrEqualOp(index, end));
      increment = sb::buildPlusAssignOp(index, stride);
    }
    else
    {
      //Create a .begin() method call on range
      SgFunctionCallExp* beginCall =
             ru::createMemberFunctionCall( "Matrix",
                                           range,
                                           "begin",
                                           ru::buildEmptyParams(),
                                           matlabFor
                                         );

      //create index = range.getMatrix().begin()
      init = beginCall;//sb::buildAssignOp(index, beginCall);

      SgFunctionCallExp *endCall =
             ru::createMemberFunctionCall( "Matrix",
                                           range,
                                           "end",
                                           ru::buildEmptyParams(),
                                           matlabFor
                                         );

      //create index != range.getMatrix().end()
      test = sb::buildExprStatement(sb::buildNotEqualOp(index, endCall));

      //++index
      increment = sb::buildPlusPlusOp(index, SgUnaryOp::prefix);

      //create *index from index
      SgExpression *dereferencedIndex = sb::buildPointerDerefExp(index);

      /*Replace each occurrence of index in the loop body with *index
        So that if in a loop:
        for i = 1:10
        y = i
        end

        then y = i will become y = *i since i in c++ is an iterator over the matrix that represents the range 1:10
      */
      ru::replaceVariable(body, isSgVarRefExp(index), dereferencedIndex);
    }

    SgScopeStatement*      scope = si::getEnclosingScope(matlabFor);
    //SgType *autoType = sb::buildIntType();

    SgType*                autoType = sb::buildOpaqueType("auto", scope);
    SgAssignInitializer*   forLoopInitializer = sb::buildAssignInitializer(init, autoType);
    SgName                 indexName = ru::nameOf(isSgVarRefExp(index));

    SgVariableDeclaration* initDeclaration =
            sb::buildVariableDeclaration( indexName,
                                          autoType,
                                          forLoopInitializer,
                                          scope
                                        );

    SgForInitStatement*    cppForInit = sb::buildForInitStatement(initDeclaration);
    SgForStatement*        cppFor = sb::buildForStatement(cppForInit, test, increment, body);

    si::replaceStatement(matlabFor, cppFor);
  }

  void transformForloop(SgProject *project)
  {
    forAllNodes(project, tfForLoop, V_SgMatlabForStatement);
  }


  //
  // MatrixOnFunctionCallArgumentsTransformer


  // \todo optimize this method to ignore function calls that do not contain any SgMatrix.
  // Currently the arguments are copied/pasted for every function call.
  static
  void tfMatrixOnFunctionCallArguments(SgFunctionCallExp* functionCall)
  {
    SgExprListExp *arguments = functionCall->get_args();

    if (arguments == NULL) return;

    SgScopeStatement *scope = si::getEnclosingScope(functionCall);

    BOOST_FOREACH(SgExpression *currentArg, arguments->get_expressions())
    {
      if( SgMatrixExp *matrix = isSgMatrixExp(currentArg)) {
        //If the argument is a matrix, change it to initializer list

        //TODO: We have to think if it is a multidimensional matrix
        //In that case, first create a variable to hold the matrix
        //and then pass the variable to the function

        Rose_STL_Container<SgExprListExp*> rows = ru::getMatrixRows(matrix);

        //I just want to work on a vector now
        ROSE_ASSERT(rows.size() == 1);

        //Convert each list of numbers [..] to a braced list {..} using AggregateInitializer
        BOOST_FOREACH(SgExprListExp *currentRow, rows)
        {
          SgAggregateInitializer *initializerList = sb::buildAggregateInitializer(currentRow);

          si::replaceExpression(currentArg, initializerList, true);
        }
      }
      else if (isSgMagicColonExp(currentArg))
      {
        //replace a SgMagicColonExp by MatlabSymbol::COLON
        SgVarRefExp *colon = sb::buildVarRefExp("MatlabSymbol::COLON", scope);

        si::replaceExpression(currentArg, colon);
      }
    }
  }


  void transformMatrixOnFunctionCallArguments(SgProject *project)
  {
    forAllNodes(project, tfMatrixOnFunctionCallArguments, V_SgFunctionCallExp);
  }


  //
  // RangeExpressionTransformer

  static
  void tfRangeExpression(SgRangeExp *rangeExp)
  {
    // Skip the range expression inside a for loop
    if (isSgMatlabForStatement(rangeExp->get_parent()))
    {
      // This is because the for loop will deal with the range in a different way.
      // Actually the range expression M:N in for loop gets transformed to a
      //   i = M; i <= N; ++i
      return;
    }

    SgStatement *enclosingStatement = si::getEnclosingStatement(rangeExp);

    //The scope where the range variable will be created
    SgScopeStatement *destinationScope = si::getEnclosingScope(rangeExp);

    if (enclosingStatement == destinationScope)
    {
      //in for loop, the expressions inside have the same enclosingStatement and scope
      destinationScope = si::getEnclosingScope(enclosingStatement);
    }

    //each variable will have a unique name
    std::string    varName = si::generateUniqueVariableName(destinationScope, "range");
    SgTypeMatrix*  matrixType = isSgTypeMatrix(fn::getInferredType(rangeExp));

    //Range<type> r
    SgVariableDeclaration* rangeVarDeclaration =
            ru::createOpaqueTemplateObject( varName,
                                                   "Range",
                                                   matrixType->get_base_type()->unparseToString(),
                                                   destinationScope
                                                 );

    si::insertStatementBefore(enclosingStatement, rangeVarDeclaration);

    SgExprListExp* functionCallArgs = ru::getExprListExpFromRangeExp(rangeExp);
    SgVarRefExp*   object = sb::buildVarRefExp(varName, destinationScope);

    //r.setBounds(1, 2, 3);
    SgFunctionCallExp* setBoundsCallExp =
            ru::createMemberFunctionCall( "Range",
                                                 object,
                                                 "setBounds",
                                                 functionCallArgs,
                                                 destinationScope
                                               );

    si::insertStatementAfter(rangeVarDeclaration, sb::buildExprStatement(setBoundsCallExp));

    //r.getMatrix()
    SgFunctionCallExp *getMatrixCallExp =
            ru::createMemberFunctionCall( "Range",
                                                 object,
                                                 "getMatrix",
                                                 sb::buildExprListExp(),
                                                 destinationScope
                                               );

    // replace 1:2:3 with r.getMatrix()
    si::replaceExpression(rangeExp, getMatrixCallExp, true);
  }

  void transformRangeExpression(SgProject *project)
  {
    forAllNodes(project, tfRangeExpression, V_SgRangeExp);
  }


  //
  // ReturnStatementTransformer

  static
  void tfReturnStmt(SgReturnStmt* returnStatement)
  {
    SgExprListExp* returnArgs = isSgExprListExp(returnStatement->get_expression());

    if (returnArgs->get_expressions().size() > 1)
    {
      //create a std::make_tuple statement
      SgScopeStatement* scope = si::getEnclosingScope(returnStatement);
      SgVarRefExp*      varref = sb::buildVarRefExp("std::make_tuple", scope);
      SgExpression*     makeTupleExp = sb::buildFunctionCallExp(varref, returnArgs);

      returnStatement->replace_expression(returnArgs, makeTupleExp);
    }
  }

  void transformReturnStatement(SgProject *project)
  {
    forAllNodes(project, tfReturnStmt, V_SgReturnStmt);
  }

  //
  // ReturnListTransformer

  static
  void tfReturnListAttribute(SgFunctionDeclaration* decl)
  {
    if (decl->getAttribute("RETURN_VARS")) return;

    SgFunctionDefinition*    def = isSgFunctionDefinition(decl->get_definition());
    if (def == NULL) return;

    SgBasicBlock*            body = def->get_body();
    ROSE_ASSERT(body);        // def has a body
    if (body->get_statements().size() == 0) return; // empty body

    SgStatement*             last = body->get_statements().back();
    SgReturnStmt*            ret = isSgReturnStmt(last);
    if (ret == NULL) return;  // not a return

    SgExprListExp*           exp = isSgExprListExp(ret->get_expression());
    if (exp == NULL) return;  // empty return

    FunctionReturnAttribute* returnAttribute = new FunctionReturnAttribute(exp);

    returnAttribute->attachTo(decl);
  }

  void transformReturnListAttribute(SgProject* project)
  {
    // alternatively we could check all last statement in a function definition
    forAllNodes(project, tfReturnListAttribute, V_SgFunctionDeclaration);
  }


  //
  // TransposeTransformer

  static
  void tfTranspose(SgMatrixTransposeOp* transposeOp)
  {
    SgExpression*      obj   = si::deepCopy(transposeOp->get_operand());
    SgExprListExp*     args  = sb::buildExprListExp(obj);
    SgScopeStatement*  scope = sg::ancestor<SgScopeStatement>(transposeOp);
    SgFunctionCallExp* call  =
           sb::buildFunctionCallExp( sb::buildVarRefExp("transpose", scope),
                                     args
                                   );

/*
    SgFunctionCallExp* call =
          ru::createMemberFunctionCall( "Matrix",
                                        obj,
                                        "t",
                                        sb::buildExprListExp(),
                                        scope
                                      );
*/

    si::replaceExpression(transposeOp, call);
  }

  void transformTranspose(SgProject* project)
  {
    forAllNodes(project, tfTranspose, V_SgMatrixTransposeOp);
  }


  //
  // MatrixOnAssignOpTransformer

  static
  void tfMatrixOnAssignOp(SgAssignOp *assignOp)
  {
    //If LHS on an assign operator is a list of expression tie it to a tuple
    //So [a, b, c] = fcnCall() changes to
    //std::tie(a, b, c) = fcnCall()

    if(SgExprListExp *assignList = isSgExprListExp(assignOp->get_lhs_operand()))
    {
      SgScopeStatement *scope = SageInterface::getEnclosingScope(assignList);
      SgFunctionCallExp *tieTuple = sb::buildFunctionCallExp(sb::buildVarRefExp("std::tie", scope), assignList);

      assignOp->set_lhs_operand(tieTuple);
    }

    /*A matrix expression on the right hand side of an equality operator
      should change to a valid c++ construct
      For now we change the matrix expression to a shift operator expresssion

      so that:
      x = [1 2; 3 4]
      becomes:
      x << 1 << 2 << endr
        << 3 << 4;
     */
    if (SgMatrixExp *matrix = isSgMatrixExp(assignOp->get_rhs_operand()))
    {
      Rose_STL_Container<SgExprListExp*> rows = ru::getMatrixRows(matrix);

      SgExpression *var = isSgVarRefExp(assignOp->get_lhs_operand());

      Rose_STL_Container<SgExprListExp*>::iterator rowsIterator = rows.begin();

      SgExprListExp* firstRow = *rowsIterator;

      SgExpressionPtrList::iterator firstRowIterator = firstRow->get_expressions().begin();
      SgExpression* firstElement = *firstRowIterator;

      SgExpression* shift = sb::buildLshiftOp(var, firstElement);

      for(firstRowIterator = firstRowIterator + 1; firstRowIterator != firstRow->get_expressions().end(); firstRowIterator++)
      {
        shift = sb::buildLshiftOp(shift, *firstRowIterator);
      }

      SgScopeStatement *scope = SageInterface::getEnclosingScope(matrix);
      SgExpression *endr = sb::buildVarRefExp("arma::endr", scope);
      shift = sb::buildLshiftOp(shift, endr);

      //Now loop through remaining rows
      for(rowsIterator = rowsIterator + 1; rowsIterator != rows.end(); rowsIterator++)
      {
        SgExpressionPtrList currentRow = (*rowsIterator)->get_expressions();

        BOOST_FOREACH(SgExpression *expr, currentRow)
        {
          shift = sb::buildLshiftOp(shift, expr);
        }

        shift = sb::buildLshiftOp(shift, endr);
      }

      //Update the parent to ignore the current assignop and replace with the shift expression
      SgStatement *expressionStatement = SageInterface::getEnclosingStatement(assignOp);
      expressionStatement->replace_expression(assignOp, shift);
    }
  }

  void transformMatrixOnAssignOp(SgProject* project)
  {
    forAllNodes(project, tfMatrixOnAssignOp, V_SgAssignOp);
  }


  //
  // ForLoopColonTransformer

#if 0
  currently we cannot typetest colon expressions

  static
  void tfForLoopColon(SgForStatement* forLoop)
  {
    //~ std::cerr << "Found a for loop ... \n";

    //colonExp is stored in increment field
    SgColonExpression* colonExp = isSgColonExpression(forLoop->get_increment());
    if (!colonExp) return;

    //~ std::cerr << colonExp->sage_class_name() << std::endl;

    SgExpression*    start = colonExp->getStart();
    SgExpression*    increment = colonExp->getIncrement();
    SgExpression*    end = colonExp->getEnd();
    SgExprStatement* exprstmt = isSgExprStatement(forLoop->get_for_init_stmt()->get_traversalSuccessorByIndex(0));
    ROSE_ASSERT(exprstmt);

    SgExpression* initVar = exprstmt->get_expression();
    SgStatement * initAssign = sb::buildAssignStatement(initVar, start);
    SgExpression* incrementAssign = sb::buildPlusAssignOp(initVar, increment);
    SgExpression* testExpression = sb::buildNotEqualOp(initVar, end);

    forLoop->set_for_init_stmt(sb::buildForInitStatement(initAssign));
    forLoop->set_increment(incrementAssign);
    forLoop->set_test(sb::buildExprStatement(testExpression));
  }
#endif

  void transformForLoopColon(SgProject* project)
  {
    // forAllNodes(project, tfForLoopColon, V_SgForStatement);
  }
} /* namespace MatlabTransformation */
