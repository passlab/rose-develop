// This test attempts to inline function calls until I cannot inline anything else or some limit is reached.
#include "rose.h"
#include <vector>
#include <string>
#include <iostream>

using namespace Rose;
using namespace std;

// Finds needle in haystack and returns true if found.  Needle is a single node (possibly an invalid pointer and will not be
// dereferenced) and haystack is the root of an abstract syntax (sub)tree.
static bool
isAstContaining(SgNode *haystack, SgNode *needle) {
    struct T1: AstSimpleProcessing {
        SgNode *needle;
        T1(SgNode *needle): needle(needle) {}
        void visit(SgNode *node) {
            if (node == needle)
                throw this;
        }
    } t1(needle);
    try {
        t1.traverse(haystack, preorder);
        return false;
    } catch (const T1*) {
        return true;
    }
}

// only call doInlinine(), without postprocessing or consistency checking. Useful to debugging things. 
static bool e_inline_only= false; 

int
main (int argc, char* argv[]) {

  // Build the project object (AST) which we will fill up with multiple files and use as a
  // handle for all processing of the AST(s) associated with one or more source files.
  std::vector<std::string> argvList(argv, argv+argc);

  // inlining only, without any post processing of AST
  if (CommandlineProcessing::isOption (argvList,"-inline-only","",true))
  {
    cout<<"Enabling inlining only mode, without any postprocessing ...."<<endl;
    e_inline_only = true;
  }
  else 
    e_inline_only = false;

  // skip calls within headers or not
  if (CommandlineProcessing::isOption (argvList,"-skip-headers","",true))
  {
    Inliner::skipHeaders = true;
    cout<<"Skipping calls within header files ...."<<endl;
  }
  else 
    Inliner::skipHeaders = false;


  SgProject* sageProject = frontend(argvList);

  AstTests::runAllTests(sageProject);
  std::vector <SgFunctionCallExp*> inlined_calls; 

  // Inline one call at a time until all have been inlined.  Loops on recursive code.
  int call_count =0; 
  size_t nInlined = 0;
  for (int count=0; count<10; ++count) {
    bool changed = false;
    BOOST_FOREACH (SgFunctionCallExp *call, SageInterface::querySubTree<SgFunctionCallExp>(sageProject)) {
      call_count++; 
      if (doInline(call)) {
        ASSERT_always_forbid2(isAstContaining(sageProject, call),
            "Inliner says it inlined, but the call expression is still present in the AST.");
        ++nInlined;
        inlined_calls.push_back(call);
        changed = true;
        break;
      }
    }
    if (!changed)
      break;
  }
  std::cout <<"Test inlined " <<StringUtility::plural(nInlined, "function calls") << " out of "<< call_count<< " calls." <<"\n";
  for (size_t i=0; i< inlined_calls.size(); i++)
  {
    std::cout <<"call@line:col " <<inlined_calls[i]->get_file_info()->get_line() <<":" << inlined_calls[i]->get_file_info()->get_col() <<"\n";
  }

  // Post-inline AST normalizations

  // DQ (6/12/2015): These functions first renames all variable (a bit heavy handed for my tastes)
  // and then (second) removes the blocks that are otherwise added to support the inlining.  The removal
  // of the blocks is the motivation for renaming the variables, but the variable renaming is 
  // done evarywhere instead of just where the functions are inlined.  I think the addition of
  // the blocks is a better solution than the overly agressive renaming of variables in the whole
  // program.  So the best solution is to comment out both of these functions.  All test codes
  // pass (including the token-based unparsing tests).
  // renameVariables(sageProject);
  // flattenBlocks(sageProject);

  if (!e_inline_only)
  {
    // This can be problematic since it tries to modifies lots of things, including codes from headers which are not modified at all. 
    cleanupInlinedCode(sageProject);
    changeAllMembersToPublic(sageProject);
    AstTests::runAllTests(sageProject);
  }

  return backend(sageProject);
}
