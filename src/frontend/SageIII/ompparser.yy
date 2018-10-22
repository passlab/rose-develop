/* OpenMP C and C++ Grammar */
/* Author: Markus Schordan, 2003 */
/* Modified by Christian Biesinger 2006 for OpenMP 2.0 */
/* Modified by Chunhua Liao for OpenMP 3.0 and connect to OmpAttribute, 2008 */
/* Updated by Chunhua Liao for OpenMP 4.5,  2017 */

/*
To debug bison conflicts, use the following command line in the build tree

/bin/sh ../../../../sourcetree/config/ylwrap ../../../../sourcetree/src/frontend/Sab.h `echo ompparser.cc | sed -e s/cc$/hh/ -e s/cpp$/hpp/ -e s/cxx$/hxx/ -e s/c++$/h++/ -e s/c$/h/` y.output ompparser.output -- bison -y -d -r state
in the build tree
*/
%name-prefix "omp_"
%defines
%error-verbose

%{
/* DQ (2/10/2014): IF is conflicting with Boost template IF. */
#undef IF

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include "sage3basic.h" // Sage Interface and Builders
#include "sageBuilder.h"
#include "OmpAttribute.h"

#ifdef _MSC_VER
  #undef IN
  #undef OUT
  #undef DUPLICATE
#endif

using namespace OmpSupport;

/* Parser - BISON */

/*the scanner function*/
extern int omp_lex(); 

/*A customized initialization function for the scanner, str is the string to be scanned.*/
extern void omp_lexer_init(const char* str);

//! Initialize the parser with the originating SgPragmaDeclaration and its pragma text
extern void omp_parser_init(SgNode* aNode, const char* str);

/*Treat the entire expression as a string for now
  Implemented in the scanner*/
extern void omp_parse_expr();

//The result AST representing the annotation
extern OmpAttribute* getParsedDirective();

static int omp_error(const char*);

//Insert variable into var_list of some clause
static bool addVar(const char* var);

static bool addVarExp(SgExpression* var);

//Insert expression into some clause
static bool addExpression(const char* expr);
static bool addUserDefinedParameter(const char* expr);

// The current AST annotation being built
static OmpAttribute* ompattribute = NULL;

// The current OpenMP construct or clause type which is being parsed
// It is automatically associated with the current ompattribute
// Used to indicate the OpenMP directive or clause to which a variable list or an expression should get added for the current OpenMP pragma being parsed.
static omp_construct_enum omptype = e_unknown;

// The node to which vars/expressions should get added
//static OmpAttribute* omptype = 0;

// The context node with the pragma annotation being parsed
//
// We attach the attribute to the pragma declaration directly for now, 
// A few OpenMP directive does not affect the next structure block
// This variable is set by the prefix_parser_init() before prefix_parse() is called.
//Liao
static SgNode* gNode;

static const char* orig_str; 

// The current expression node being generated 
static SgExpression* current_exp = NULL;
bool b_within_variable_list  = false;  // a flag to indicate if the program is now processing a list of variables

// We now follow the OpenMP 4.0 standard's C-style array section syntax: [lower-bound:length] or just [length]
// the latest variable symbol being parsed, used to help parsing the array dimensions associated with array symbol
// such as a[0:n][0:m]
static SgVariableSymbol* array_symbol; 
static SgExpression* lower_exp = NULL;
static SgExpression* length_exp = NULL;
// check if the parsed a[][] is an array element access a[i][j] or array section a[lower:length][lower:length]
// 
static bool arraySection=true; 

// mark whether it is complex clause.
static bool is_complex_clause = false;

static bool addComplexVar(const char* var);

%}

%locations

/* The %union declaration specifies the entire collection of possible data types for semantic values. these names are used in the %token and %type declarations to pick one of the types for a terminal or nonterminal symbol
corresponding C type is union name defaults to YYSTYPE.
*/

%union {  int itype;
          double ftype;
          const char* stype;
          void* ptype; /* For expressions */
        }

/*Some operators have a suffix 2 to avoid name conflicts with ROSE's existing types, We may want to reuse them if it is proper. 
  experimental BEGIN END are defined by default, we use TARGET_BEGIN TARGET_END instead. 
  Liao*/
%token  OMP PARALLEL IF NUM_THREADS ORDERED SCHEDULE STATIC DYNAMIC GUIDED RUNTIME SECTIONS SINGLE NOWAIT SECTION
        FOR MASTER CRITICAL BARRIER ATOMIC FLUSH TARGET UPDATE DIST_DATA BLOCK DUPLICATE CYCLIC
        THREADPRIVATE PRIVATE COPYPRIVATE FIRSTPRIVATE LASTPRIVATE SHARED DEFAULT NONE REDUCTION COPYIN ALLOCATE
        TASK TASKWAIT UNTIED COLLAPSE AUTO DECLARE DATA DEVICE MAP ALLOC TO FROM TOFROM PROC_BIND CLOSE SPREAD
        SIMD SAFELEN ALIGNED LINEAR UNIFORM INBRANCH NOTINBRANCH MPI MPI_ALL MPI_MASTER TARGET_BEGIN TARGET_END
        '(' ')' ',' ':' '+' '*' '-' '&' '^' '|' LOGAND LOGOR SHLEFT SHRIGHT PLUSPLUS MINUSMINUS PTR_TO '.'
        LE_OP2 GE_OP2 EQ_OP2 NE_OP2 RIGHT_ASSIGN2 LEFT_ASSIGN2 ADD_ASSIGN2
        SUB_ASSIGN2 MUL_ASSIGN2 DIV_ASSIGN2 MOD_ASSIGN2 AND_ASSIGN2 
        XOR_ASSIGN2 OR_ASSIGN2 DEPEND IN OUT INOUT MERGEABLE
        LEXICALERROR IDENTIFIER MIN MAX
        READ WRITE CAPTURE SIMDLEN FINAL PRIORITY
        INSCAN
/*We ignore NEWLINE since we only care about the pragma string , We relax the syntax check by allowing it as part of line continuation */
%token <itype> ICONSTANT   
%token <stype> EXPRESSION ID_EXPRESSION 

/* associativity and precedence */
%left '<' '>' '=' "!=" "<=" ">="
%left '+' '-'
%left '*' '/' '%'

/* nonterminals names, types for semantic values, only for nonterminals representing expressions!! not for clauses with expressions.
 */
%type <ptype> expression assignment_expr conditional_expr 
              logical_or_expr logical_and_expr
              inclusive_or_expr exclusive_or_expr and_expr
              equality_expr relational_expr 
              shift_expr additive_expr multiplicative_expr 
              primary_expr unary_expr postfix_expr variable_exp_list
%type <itype> schedule_kind

/* start point for the parsing */
%start openmp_directive

%%

/* NOTE: We can't use the EXPRESSION lexer token directly. Instead, we have
 * to first call omp_parse_expr, because we parse up to the terminating
 * paren.
 */

openmp_directive : parallel_directive 
                 | for_directive
                 | for_simd_directive
                 | declare_simd_directive
                 | sections_directive
                 | single_directive
                 | parallel_for_directive
                 | parallel_for_simd_directive
                 | parallel_sections_directive
                 | task_directive
                 | master_directive
                 | critical_directive
                 | atomic_directive
                 | ordered_directive
                 | barrier_directive 
                 | taskwait_directive
                 | flush_directive
                 | threadprivate_directive
                 | section_directive
                 | target_directive
                 | target_data_directive
                 | simd_directive
                 ;

parallel_directive : /* #pragma */ OMP PARALLEL {
                       ompattribute = buildOmpAttribute(e_parallel,gNode,true);
                       omptype = e_parallel; 
                       cur_omp_directive=omptype;
                     }
                     parallel_clause_optseq 
                   ;

parallel_clause_optseq : /* empty */
                       | parallel_clause_seq
                       ;

parallel_clause_seq : parallel_clause
                    | parallel_clause_seq parallel_clause
                    | parallel_clause_seq ',' parallel_clause
                    ;

proc_bind_clause : PROC_BIND '(' MASTER ')' { 
                        ompattribute->addClause(e_proc_bind);
                        ompattribute->setProcBindPolicy (e_proc_bind_master); 
                      }
                    | PROC_BIND '(' CLOSE ')' {
                        ompattribute->addClause(e_proc_bind);
                        ompattribute->setProcBindPolicy (e_proc_bind_close); 
                      }
                    | PROC_BIND '(' SPREAD ')' {
                        ompattribute->addClause(e_proc_bind);
                        ompattribute->setProcBindPolicy (e_proc_bind_spread); 
                      }
                    ;

/*  follow the order in the 4.5 specification  */ 
parallel_clause : if_clause
                | num_threads_clause
                | default_clause
                | private_clause
                | firstprivate_clause
                | share_clause
                | copyin_clause
                | reduction_clause
                | proc_bind_clause
                ;

copyin_clause: COPYIN {
                           ompattribute->addClause(e_copyin);
                           omptype = e_copyin;
                         } '(' {b_within_variable_list = true;} variable_list ')' {b_within_variable_list = false;}
                ;


for_directive : /* #pragma */ OMP FOR { 
                  ompattribute = buildOmpAttribute(e_for,gNode,true); 
                  omptype = e_for; 
                  cur_omp_directive=omptype;
                }
                for_clause_optseq
              ;

for_clause_optseq: /* emtpy */
              | for_clause_seq
              ;

for_clause_seq: for_clause
              | for_clause_seq for_clause 
              | for_clause_seq ',' for_clause

/*  updated to 4.5 */
for_clause: private_clause
           | firstprivate_clause
           | lastprivate_clause
           | linear_clause
           | reduction_clause
           | schedule_clause
           | collapse_clause
           | ordered_clause
           | nowait_clause  
          ; 

/* use this for the combined for simd directive */
for_or_simd_clause : ordered_clause
           | schedule_clause
           | private_clause
           | firstprivate_clause
           | lastprivate_clause
           | reduction_clause
           | collapse_clause
           | unique_simd_clause
           | nowait_clause  
          ;

schedule_chunk_opt: /* empty */
                | ',' expression { 
                     addExpression("");
                 }
                ; 

ordered_clause: ORDERED {
                      ompattribute->addClause(e_ordered_clause);
                      omptype = e_ordered_clause;
                } ordered_parameter_opt
               ;

ordered_parameter_opt: /* empty */
                | '(' expression ')' {
                    addExpression("");
                   }
                 ;

schedule_clause: SCHEDULE '(' schedule_kind {
                      ompattribute->addClause(e_schedule);
                      ompattribute->setScheduleKind(static_cast<omp_construct_enum>($3));
                      omptype = e_schedule; }
                    schedule_chunk_opt  ')' 
                 ;

collapse_clause: COLLAPSE {
                      ompattribute->addClause(e_collapse);
                      omptype = e_collapse;
                    } '(' expression ')' { 
                      addExpression("");
                    }
                  ;
 
schedule_kind : STATIC  { $$ = e_schedule_static; }
              | DYNAMIC { $$ = e_schedule_dynamic; }
              | GUIDED  { $$ = e_schedule_guided; }
              | AUTO    { $$ = e_schedule_auto; }
              | RUNTIME { $$ = e_schedule_runtime; }
              ;

sections_directive : /* #pragma */ OMP SECTIONS { 
                       ompattribute = buildOmpAttribute(e_sections,gNode, true); 
                     } sections_clause_optseq
                   ;

sections_clause_optseq : /* empty */
                       | sections_clause_seq
                       ;

sections_clause_seq : sections_clause
                    | sections_clause_seq sections_clause
                    | sections_clause_seq ',' sections_clause
                    ;

sections_clause : private_clause
                | firstprivate_clause
                | lastprivate_clause
                | reduction_clause
                | nowait_clause
                ;

section_directive : /* #pragma */  OMP SECTION { 
                      ompattribute = buildOmpAttribute(e_section,gNode,true); 
                    }
                  ;

single_directive : /* #pragma */ OMP SINGLE { 
                     ompattribute = buildOmpAttribute(e_single,gNode,true); 
                     omptype = e_single; 
                   } single_clause_optseq
                 ;

single_clause_optseq : /* empty */
                     | single_clause_seq
                     ;

single_clause_seq : single_clause
                  | single_clause_seq single_clause
                  | single_clause_seq ',' single_clause
                  ;
nowait_clause: NOWAIT {
                  ompattribute->addClause(e_nowait);
                }
              ;

single_clause : unique_single_clause
              | private_clause
              | firstprivate_clause
              | nowait_clause
              ;
unique_single_clause : COPYPRIVATE { 
                         ompattribute->addClause(e_copyprivate);
                         omptype = e_copyprivate; 
                       }
                       '(' {b_within_variable_list = true;} variable_list ')' {b_within_variable_list =false;}

task_directive : /* #pragma */ OMP TASK {
                   ompattribute = buildOmpAttribute(e_task,gNode,true);
                   omptype = e_task; 
                   cur_omp_directive = omptype; 
                 } task_clause_optseq
               ;

task_clause_optseq :  /* empty */ 
                   | task_clause_seq 
                   ; 

task_clause_seq    : task_clause
                   | task_clause_seq task_clause
                   | task_clause_seq ',' task_clause
                   ;

task_clause : unique_task_clause
            | default_clause
            | private_clause
            | firstprivate_clause
            | share_clause
            | depend_clause
            | if_clause
            ;

unique_task_clause : FINAL { 
                       ompattribute->addClause(e_final);
                       omptype = e_final; 
                     } '(' expression ')' { 
                       addExpression("");
                     }
                   | PRIORITY { 
                       ompattribute->addClause(e_priority);
                       omptype = e_priority; 
                     } '(' expression ')' { 
                       addExpression("");
                     }
                   | UNTIED {
                       ompattribute->addClause(e_untied);
                     }
                   | MERGEABLE {
                       ompattribute->addClause(e_mergeable);
                     }
                   ;
                   
depend_clause : DEPEND { 
                          ompattribute->addClause(e_depend);
                        } '(' dependence_type ':' {b_within_variable_list = true; array_symbol=NULL; } variable_exp_list ')' 
                        {
                          assert ((ompattribute->getVariableList(omptype)).size()>0); /* I believe that depend() must have variables */
                          b_within_variable_list = false;
                        }
                      ;

dependence_type : IN {
                       ompattribute->setDependenceType(e_depend_in); 
                       omptype = e_depend_in; /*variables are stored for each operator*/
                     }
                   | OUT {
                       ompattribute->setDependenceType(e_depend_out);  
                       omptype = e_depend_out;
                     }
                   | INOUT {
                       ompattribute->setDependenceType(e_depend_inout); 
                       omptype = e_depend_inout;
                      }
                   ;


parallel_for_directive : /* #pragma */ OMP PARALLEL FOR { 
                           ompattribute = buildOmpAttribute(e_parallel_for,gNode, true); 
                           omptype=e_parallel_for; 
                           cur_omp_directive = omptype;
                         } parallel_for_clauseoptseq
                       ;

parallel_for_clauseoptseq : /* empty */
                          | parallel_for_clause_seq
                          ;

parallel_for_clause_seq : parallel_for_clause
                        | parallel_for_clause_seq parallel_for_clause
                        | parallel_for_clause_seq ',' parallel_for_clause
                        ;
/*
clause can be any of the clauses accepted by the parallel or for directives, except the
nowait clause, updated for 4.5.
*/
parallel_for_clause : if_clause
                    | num_threads_clause
                    | default_clause
                    | private_clause
                    | firstprivate_clause
                    | share_clause
                    | copyin_clause
                    | reduction_clause
                    | proc_bind_clause
                    | lastprivate_clause
                    | linear_clause
                    | schedule_clause 
                    | collapse_clause
                    | ordered_clause
                   ;

parallel_for_simd_directive : /* #pragma */ OMP PARALLEL FOR SIMD { 
                           ompattribute = buildOmpAttribute(e_parallel_for_simd, gNode, true); 
                           omptype= e_parallel_for_simd;
                           cur_omp_directive = omptype;
                         } parallel_for_simd_clauseoptseq
                       ;

parallel_for_simd_clauseoptseq : /* empty */
                          | parallel_for_simd_clause_seq

parallel_for_simd_clause_seq : parallel_for_simd_clause
                        | parallel_for_simd_clause_seq parallel_for_simd_clause
                        | parallel_for_simd_clause_seq ',' parallel_for_simd_clause
                          
parallel_for_simd_clause: copyin_clause
                    | ordered_clause
                    | schedule_clause
                    | unique_simd_clause
                    | default_clause
                    | private_clause
                    | firstprivate_clause
                    | lastprivate_clause
                    | reduction_clause
                    | collapse_clause
                    | share_clause
                    | if_clause
                    | num_threads_clause
                    | proc_bind_clause
                   ; 
 
parallel_sections_directive : /* #pragma */ OMP PARALLEL SECTIONS { 
                                ompattribute =buildOmpAttribute(e_parallel_sections,gNode, true); 
                                omptype = e_parallel_sections; 
                                cur_omp_directive = omptype;
                              } parallel_sections_clause_optseq
                            ;

parallel_sections_clause_optseq : /* empty */
                                | parallel_sections_clause_seq
                                ;

parallel_sections_clause_seq : parallel_sections_clause
                             | parallel_sections_clause_seq parallel_sections_clause
                             | parallel_sections_clause_seq ',' parallel_sections_clause
                             ;

parallel_sections_clause : copyin_clause
                         | default_clause
                         | private_clause
                         | firstprivate_clause
                         | lastprivate_clause
                         | share_clause
                         | reduction_clause
                         | if_clause
                         | num_threads_clause
                         | proc_bind_clause
                         ;

master_directive : /* #pragma */ OMP MASTER { 
                     ompattribute = buildOmpAttribute(e_master, gNode, true);
                     cur_omp_directive = e_master; 
}
                 ;

critical_directive : /* #pragma */ OMP CRITICAL {
                       ompattribute = buildOmpAttribute(e_critical, gNode, true); 
                       cur_omp_directive = e_critical;
                     } region_phraseopt
                   ;

region_phraseopt : /* empty */
                 | region_phrase
                 ;

/* This used to use IDENTIFIER, but our lexer does not ever return that:
 * Things that'd match it are, instead, ID_EXPRESSION. So use that here.
 * named critical section
 */
region_phrase : '(' ID_EXPRESSION ')' { 
                  ompattribute->setCriticalName((const char*)$2);
                }
              ;

barrier_directive : /* #pragma */ OMP BARRIER { 
                      ompattribute = buildOmpAttribute(e_barrier,gNode, true); 
                      cur_omp_directive = e_barrier;
}
                  ;

taskwait_directive : /* #pragma */ OMP TASKWAIT { 
                       ompattribute = buildOmpAttribute(e_taskwait, gNode, true);  
                       cur_omp_directive = e_taskwait;
                       }
                   ;

atomic_directive : /* #pragma */ OMP ATOMIC { 
                     ompattribute = buildOmpAttribute(e_atomic,gNode, true); 
                     cur_omp_directive = e_atomic;
                     } atomic_clauseopt
                 ;

atomic_clauseopt : /* empty */
                 | atomic_clause
                 ;

atomic_clause : READ { ompattribute->addClause(e_atomic_clause);
                       ompattribute->setAtomicAtomicity(e_atomic_read);
                      }
               | WRITE{ ompattribute->addClause(e_atomic_clause);
                       ompattribute->setAtomicAtomicity(e_atomic_write);
                  }

               | UPDATE { ompattribute->addClause(e_atomic_clause);
                       ompattribute->setAtomicAtomicity(e_atomic_update);
                  }
               | CAPTURE { ompattribute->addClause(e_atomic_clause);
                       ompattribute->setAtomicAtomicity(e_atomic_capture);
                  }
                ;
flush_directive : /* #pragma */ OMP FLUSH {
                    ompattribute = buildOmpAttribute(e_flush,gNode, true);
                    omptype = e_flush; 
                    cur_omp_directive = omptype;
                  } flush_varsopt
                ;

flush_varsopt : /* empty */
              | flush_vars
              ;

flush_vars : '(' {b_within_variable_list = true;} variable_list ')' {b_within_variable_list = false;}
           ;

ordered_directive : /* #pragma */ OMP ORDERED { 
                      ompattribute = buildOmpAttribute(e_ordered_directive,gNode, true); 
                      cur_omp_directive = e_ordered_directive;
                    }
                  ;

threadprivate_directive : /* #pragma */ OMP THREADPRIVATE {
                            ompattribute = buildOmpAttribute(e_threadprivate,gNode, true); 
                            omptype = e_threadprivate; 
                            cur_omp_directive = omptype;
                          } '(' {b_within_variable_list = true;} variable_list ')' {b_within_variable_list = false;}
                        ;

default_clause : DEFAULT '(' SHARED ')' { 
                        ompattribute->addClause(e_default);
                        ompattribute->setDefaultValue(e_default_shared); 
                      }
                    | DEFAULT '(' NONE ')' {
                        ompattribute->addClause(e_default);
                        ompattribute->setDefaultValue(e_default_none);
                      }
                    ;

                   
private_clause : PRIVATE {
                              ompattribute->addClause(e_private); omptype = e_private;
                            } '(' {b_within_variable_list = true;} variable_list ')' {b_within_variable_list = false;}
                          ;

firstprivate_clause : FIRSTPRIVATE { 
                                 ompattribute->addClause(e_firstprivate); 
                                 omptype = e_firstprivate;
                               } '(' {b_within_variable_list = true;} variable_list ')' {b_within_variable_list = false;}
                             ;

lastprivate_clause : LASTPRIVATE { 
                                  ompattribute->addClause(e_lastprivate); 
                                  omptype = e_lastprivate;
                                } '(' {b_within_variable_list = true;} variable_list ')' {b_within_variable_list = false;}
                              ;

share_clause : SHARED {
                        ompattribute->addClause(e_shared); omptype = e_shared; 
                      } '(' {b_within_variable_list = true;} variable_list ')' {b_within_variable_list = false;}
                    ;

reduction_clause : REDUCTION { 
                        ompattribute->addComplexClauseParameters(e_reduction);
                        omptype = e_reduction;
                        is_complex_clause = true;
                        } '(' reduction_parameters {is_complex_clause = false;} ')'
                      ;

reduction_parameters: reduction_modifier ',' {
                        } reduction_identifier ':' {b_within_variable_list = true;} variable_list {b_within_variable_list = false;
                        }
                        | reduction_identifier ':' {b_within_variable_list = true;} variable_list {b_within_variable_list = false;
                        }
            ;

reduction_modifier : INSCAN { ompattribute->setComplexClauseFirstParameter(e_reduction_inscan); }
            | TASK { ompattribute->setComplexClauseFirstParameter(e_reduction_task); }
            | DEFAULT { ompattribute->setComplexClauseFirstParameter(e_reduction_default); }
            ;


reduction_identifier : '+' {
                        ompattribute->setComplexClauseSecondParameter(e_reduction_plus);
                     }
                   | '*' {
                       ompattribute->setComplexClauseSecondParameter(e_reduction_mul);  
                     }
                   | '-' {
                       ompattribute->setComplexClauseSecondParameter(e_reduction_minus); 
                      }
                   | MIN {
                       ompattribute->setReductionOperator(e_reduction_min); 
                       omptype = e_reduction_min;
                      }
                   | MAX {
                       ompattribute->setReductionOperator(e_reduction_max); 
                       omptype = e_reduction_max;
                      }
                   | '&' {
                       ompattribute->setComplexClauseSecondParameter(e_reduction_bitand);  
                      }
                   | '^' {
                       ompattribute->setComplexClauseSecondParameter(e_reduction_bitxor);  
                      }
                   | '|' {
                       ompattribute->setComplexClauseSecondParameter(e_reduction_bitor);  
                      }
                   | LOGAND /* && */ {
                       ompattribute->setComplexClauseSecondParameter(e_reduction_logand);  
                     }
                   | LOGOR /* || */ {
                       ompattribute->setComplexClauseSecondParameter(e_reduction_logor); 
                     }
                   | expression {
                       ompattribute->setComplexClauseSecondParameter(e_reduction_user_defined_identifier);
                       addUserDefinedParameter("");
                    }
                   ;

target_data_directive: /* pragma */ OMP TARGET DATA {
                       ompattribute = buildOmpAttribute(e_target_data, gNode,true);
                       omptype = e_target_data;
                     }
                      target_data_clause_seq
                    ;

target_data_clause_seq : target_data_clause
                    | target_data_clause_seq target_data_clause
                    | target_data_clause_seq ',' target_data_clause
                    ;

target_data_clause : device_clause 
                | map_clause
                | if_clause
                ;

target_directive: /* #pragma */ OMP TARGET {
                       ompattribute = buildOmpAttribute(e_target,gNode,true);
                       omptype = e_target;
                       cur_omp_directive = omptype;
                     }
                     target_clause_optseq 
                   ;

target_clause_optseq : /* empty */
                       | target_clause_seq
                       ;

target_clause_seq : target_clause
                    | target_clause_seq target_clause
                    | target_clause_seq ',' target_clause
                    ;

target_clause : device_clause 
                | map_clause
                | if_clause
                | num_threads_clause
                | begin_clause
                | end_clause
                ;
/*
device_clause : DEVICE {
                           ompattribute->addClause(e_device);
                           omptype = e_device;
                         } '(' expression ')' {
                           addExpression("");
                         }
                ;
*/

/* Experimental extensions to support multiple devices and MPI */
device_clause : DEVICE {
                           ompattribute->addClause(e_device);
                           omptype = e_device;
                         } '(' expression_or_star_or_mpi 
                ;
expression_or_star_or_mpi: 
                  MPI ')' { // special mpi device for supporting MPI code generation
                            current_exp= SageBuilder::buildStringVal("mpi");
                            addExpression("mpi");
                          }
                  | MPI_ALL ')' { // special mpi device for supporting MPI code generation
                            current_exp= SageBuilder::buildStringVal("mpi:all");
                            addExpression("mpi:all");
                          }
                  | MPI_MASTER ')' { // special mpi device for supporting MPI code generation
                            current_exp= SageBuilder::buildStringVal("mpi:master");
                            addExpression("mpi:master");
                          }
                  | expression ')' { //normal expression
                           addExpression("");
                          }
                  | '*' ')' { // our extension device (*) 
                            current_exp= SageBuilder::buildCharVal('*'); 
                            addExpression("*");  }; 


begin_clause: TARGET_BEGIN {
                           ompattribute->addClause(e_begin);
                           omptype = e_begin;
                    }
                    ;

end_clause: TARGET_END {
                           ompattribute->addClause(e_end);
                           omptype = e_end;
                    }
                    ;

                    
if_clause: IF {
                           ompattribute->addClause(e_if);
                           omptype = e_if;
             } '(' clause_with_opt_attribute ')' {
                        //    addExpression("");
             }
             ;

clause_with_opt_attribute: opt_attribute ':' expression {
                        addExpression("");
                        }
                      | expression {
                        addExpression("");
                        }
            ;

opt_attribute: PARALLEL {
             ;
            }
            ;

num_threads_clause: NUM_THREADS {
                           ompattribute->addClause(e_num_threads);
                           omptype = e_num_threads;
                         } '(' expression ')' {
                            addExpression("");
                         }
                      ;
map_clause: MAP {
                          ompattribute->addClause(e_map);
                           omptype = e_map; // use as a flag to see if it will be reset later
                     } '(' map_clause_optseq 
                     { 
                       b_within_variable_list = true;
                       if (omptype == e_map) // map data directions are not explicitly specified
                       {
                          ompattribute->setMapVariant(e_map_tofrom);  omptype = e_map_tofrom;  
                       }
                     } 
                     map_variable_list ')' { b_within_variable_list =false;} 

map_clause_optseq: /* empty, default to be tofrom*/ { ompattribute->setMapVariant(e_map_tofrom);  omptype = e_map_tofrom; /*No effect here???*/ }
                    | ALLOC ':' { ompattribute->setMapVariant(e_map_alloc);  omptype = e_map_alloc; } 
                    | TO     ':' { ompattribute->setMapVariant(e_map_to); omptype = e_map_to; } 
                    | FROM    ':' { ompattribute->setMapVariant(e_map_from); omptype = e_map_from; } 
                    | TOFROM  ':' { ompattribute->setMapVariant(e_map_tofrom); omptype = e_map_tofrom; } 
                    ;

for_simd_directive : /* #pragma */ OMP FOR SIMD { 
                  ompattribute = buildOmpAttribute(e_for_simd, gNode,true); 
                  cur_omp_directive = e_for_simd;
                }
                for_or_simd_clause_optseq
              ;


for_or_simd_clause_optseq:  /* empty*/
                      | for_or_simd_clause_seq
                      ;

simd_directive: /* # pragma */ OMP SIMD
                  { ompattribute = buildOmpAttribute(e_simd,gNode,true); 
                    omptype = e_simd; 
                    cur_omp_directive = omptype;
                    }
                   simd_clause_optseq
                ;

simd_clause_optseq: /*empty*/
             | simd_clause_seq 
            ;

simd_clause_seq: simd_clause
               |  simd_clause_seq simd_clause 
               |  simd_clause_seq ',' simd_clause 
              ;

/* updated to 4.5 */
simd_clause: safelen_clause
           | simdlen_clause
           | linear_clause
           | aligned_clause
           | private_clause
           | lastprivate_clause
           | reduction_clause
           | collapse_clause
            ;


for_or_simd_clause_seq
                : for_or_simd_clause
                | for_or_simd_clause_seq for_or_simd_clause
                | for_or_simd_clause_seq ',' for_or_simd_clause
                ;

safelen_clause :  SAFELEN {
                        ompattribute->addClause(e_safelen);
                        omptype = e_safelen;
                      } '(' expression ')' {
                        addExpression("");
                 }
                ; 

unique_simd_clause: safelen_clause
                | simdlen_clause
                | aligned_clause
                | linear_clause
                ;

simdlen_clause: SIMDLEN {
                          ompattribute->addClause(e_simdlen);
                          omptype = e_simdlen;
                          } '(' expression ')' {
                          addExpression(""); 
                      } 
                  ;

declare_simd_directive: OMP DECLARE SIMD {
                        ompattribute = buildOmpAttribute(e_declare_simd, gNode,true);
                        cur_omp_directive = e_declare_simd;
                     }
                     declare_simd_clause_optseq
                     ;

declare_simd_clause_optseq : /* empty*/
                        | declare_simd_clause_seq
                        ;

declare_simd_clause_seq
                : declare_simd_clause
                | declare_simd_clause_seq declare_simd_clause
                | declare_simd_clause_seq ',' declare_simd_clause
                ; 

declare_simd_clause     : simdlen_clause
                | linear_clause
                | aligned_clause
                | uniform_clause
                | INBRANCH { ompattribute->addClause(e_inbranch); omptype = e_inbranch; /*TODO: this is temporary, to be moved to declare simd */}
                | NOTINBRANCH { ompattribute->addClause(e_notinbranch); omptype = e_notinbranch; /*TODO: this is temporary, to be moved to declare simd */ }
              ;

uniform_clause : UNIFORM { 
                         ompattribute->addClause(e_uniform);
                         omptype = e_uniform; 
                       }
                       '(' {b_within_variable_list = true;} variable_list ')' {b_within_variable_list =false;}
                ;

aligned_clause : ALIGNED { 
                         ompattribute->addClause(e_aligned);
                         omptype = e_aligned; 
                       }
                       '(' {b_within_variable_list = true;} variable_list {b_within_variable_list =false;} aligned_clause_optseq ')'
               ;
aligned_clause_optseq: /* empty */
                        | aligned_clause_alignment
                        ;

aligned_clause_alignment: ':' expression {addExpression(""); } 


linear_clause :  LINEAR { 
                         ompattribute->addClause(e_linear);
                         omptype = e_linear; 
                        }
                       '(' {b_within_variable_list = true;} variable_list {b_within_variable_list =false;}  linear_clause_step_optseq ')'
                ;

linear_clause_step_optseq: /* empty */
                        | linear_clause_step
                        ;

linear_clause_step: ':' expression {addExpression(""); } 

/* parsing real expressions here, Liao, 10/12/2008
   */       
/* expression: { omp_parse_expr(); } EXPRESSION { if (!addExpression((const char*)$2)) YYABORT; }
*/
/* Sara Royuela, 04/27/2012
 * Extending grammar to accept conditional expressions, arithmetic and bitwise expressions and member accesses
 */
expression : assignment_expr

assignment_expr : conditional_expr
                | logical_or_expr 
                | unary_expr '=' assignment_expr  {
                    current_exp = SageBuilder::buildAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                | unary_expr RIGHT_ASSIGN2 assignment_expr {
                    current_exp = SageBuilder::buildRshiftAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                | unary_expr LEFT_ASSIGN2 assignment_expr {
                    current_exp = SageBuilder::buildLshiftAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                | unary_expr ADD_ASSIGN2 assignment_expr {
                    current_exp = SageBuilder::buildPlusAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                | unary_expr SUB_ASSIGN2 assignment_expr {
                    current_exp = SageBuilder::buildMinusAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                | unary_expr MUL_ASSIGN2 assignment_expr {
                    current_exp = SageBuilder::buildMultAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                | unary_expr DIV_ASSIGN2 assignment_expr {
                    current_exp = SageBuilder::buildDivAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                | unary_expr MOD_ASSIGN2 assignment_expr {
                    current_exp = SageBuilder::buildModAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                | unary_expr AND_ASSIGN2 assignment_expr {
                    current_exp = SageBuilder::buildAndAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                | unary_expr XOR_ASSIGN2 assignment_expr {
                    current_exp = SageBuilder::buildXorAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                | unary_expr OR_ASSIGN2 assignment_expr {
                    current_exp = SageBuilder::buildIorAssignOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp;
                  }
                ;

conditional_expr : logical_or_expr '?' assignment_expr ':' assignment_expr {
                     current_exp = SageBuilder::buildConditionalExp(
                       (SgExpression*)($1),
                       (SgExpression*)($3),
                       (SgExpression*)($5)
                     );
                     $$ = current_exp;
                   }
                 ;

logical_or_expr : logical_and_expr
                | logical_or_expr LOGOR logical_and_expr {
                    current_exp = SageBuilder::buildOrOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    );
                    $$ = current_exp;
                  }
                ;

logical_and_expr : inclusive_or_expr
                 | logical_and_expr LOGAND inclusive_or_expr {
                     current_exp = SageBuilder::buildAndOp(
                       (SgExpression*)($1),
                       (SgExpression*)($3)
                     );
                   $$ = current_exp;
                 }
                 ;

inclusive_or_expr : exclusive_or_expr
                  | inclusive_or_expr '|' exclusive_or_expr {
                      current_exp = SageBuilder::buildBitOrOp(
                        (SgExpression*)($1),
                        (SgExpression*)($3)
                      );
                      $$ = current_exp;
                    }
                  ;

exclusive_or_expr : and_expr
                  | exclusive_or_expr '^' and_expr {
                      current_exp = SageBuilder::buildBitXorOp(
                        (SgExpression*)($1),
                        (SgExpression*)($3)
                      );
                      $$ = current_exp;
                    }
                  ;

and_expr : equality_expr
         | and_expr '&' equality_expr {
             current_exp = SageBuilder::buildBitAndOp(
               (SgExpression*)($1),
               (SgExpression*)($3)
             );
             $$ = current_exp;
           }
         ;  

equality_expr : relational_expr
              | equality_expr EQ_OP2 relational_expr {
                  current_exp = SageBuilder::buildEqualityOp(
                    (SgExpression*)($1),
                    (SgExpression*)($3)
                  ); 
                  $$ = current_exp;
                }
              | equality_expr NE_OP2 relational_expr {
                  current_exp = SageBuilder::buildNotEqualOp(
                    (SgExpression*)($1),
                    (SgExpression*)($3)
                  ); 
                  $$ = current_exp;
                }
              ;
              
relational_expr : shift_expr
                | relational_expr '<' shift_expr { 
                    current_exp = SageBuilder::buildLessThanOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp; 
                  // std::cout<<"debug: buildLessThanOp():\n"<<current_exp->unparseToString()<<std::endl;
                  }
                | relational_expr '>' shift_expr {
                    current_exp = SageBuilder::buildGreaterThanOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp; 
                  }
                | relational_expr LE_OP2 shift_expr {
                    current_exp = SageBuilder::buildLessOrEqualOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    ); 
                    $$ = current_exp; 
                  }
                | relational_expr GE_OP2 shift_expr {
                    current_exp = SageBuilder::buildGreaterOrEqualOp(
                      (SgExpression*)($1),
                      (SgExpression*)($3)
                    );
                    $$ = current_exp; 
                  }
                ;

shift_expr : additive_expr
           | shift_expr SHRIGHT additive_expr {
               current_exp = SageBuilder::buildRshiftOp(
                 (SgExpression*)($1),
                 (SgExpression*)($3)
               ); 
               $$ = current_exp; 
             }
           | shift_expr SHLEFT additive_expr {
               current_exp = SageBuilder::buildLshiftOp(
                 (SgExpression*)($1),
                 (SgExpression*)($3)
               ); 
               $$ = current_exp; 
             }
           ;

additive_expr : multiplicative_expr
              | additive_expr '+' multiplicative_expr {
                  current_exp = SageBuilder::buildAddOp(
                    (SgExpression*)($1),
                    (SgExpression*)($3)
                  ); 
                  $$ = current_exp; 
                }
              | additive_expr '-' multiplicative_expr {
                  current_exp = SageBuilder::buildSubtractOp(
                    (SgExpression*)($1),
                    (SgExpression*)($3)
                  ); 
                  $$ = current_exp; 
                }
              ;

multiplicative_expr : primary_expr
                    | multiplicative_expr '*' primary_expr {
                        current_exp = SageBuilder::buildMultiplyOp(
                          (SgExpression*)($1),
                          (SgExpression*)($3)
                        ); 
                        $$ = current_exp; 
                      }
                    | multiplicative_expr '/' primary_expr {
                        current_exp = SageBuilder::buildDivideOp(
                          (SgExpression*)($1),
                          (SgExpression*)($3)
                        ); 
                        $$ = current_exp; 
                      }
                    | multiplicative_expr '%' primary_expr {
                        current_exp = SageBuilder::buildModOp(
                          (SgExpression*)($1),
                          (SgExpression*)($3)
                        ); 
                        $$ = current_exp; 
                      }
                    ;

primary_expr : ICONSTANT {
               current_exp = SageBuilder::buildIntVal($1);
               $$ = current_exp;
              }
             | ID_EXPRESSION {
               current_exp = SageBuilder::buildVarRefExp(
                 (const char*)($1),SageInterface::getScope(gNode)
               );
               $$ = current_exp;
              }
             | '(' expression ')' {
                 $$ = current_exp;
               } 
             ;

unary_expr : postfix_expr {
             current_exp = (SgExpression*)($1);
             $$ = current_exp;
            }  
           |PLUSPLUS unary_expr {
              current_exp = SageBuilder::buildPlusPlusOp(
                (SgExpression*)($2),
                SgUnaryOp::prefix
              );
              $$ = current_exp;
            }
          | MINUSMINUS unary_expr {
              current_exp = SageBuilder::buildMinusMinusOp(
                (SgExpression*)($2),
                SgUnaryOp::prefix
              );
              $$ = current_exp;
            }

           ;
/* Follow ANSI-C yacc grammar */                
postfix_expr:primary_expr {
               arraySection= false; 
                 current_exp = (SgExpression*)($1);
                 $$ = current_exp;
             }
            |postfix_expr '[' expression ']' {
               arraySection= false; 
               current_exp = SageBuilder::buildPntrArrRefExp((SgExpression*)($1), (SgExpression*)($3));
               $$ = current_exp;
             }
            | postfix_expr '[' expression ':' expression ']'
             {
               arraySection= true; // array section // TODO; BEST solution: still need a tree here!!
               // only add  symbol to the attribute for this first time 
               // postfix_expr should be ID_EXPRESSION
               if (!array_symbol)
               {  
                 SgVarRefExp* vref = isSgVarRefExp((SgExpression*)($1));
                 assert (vref);
                 array_symbol = ompattribute->addVariable(omptype, vref->unparseToString());
                 // if (!addVar((const char*) )) YYABORT;
                 //std::cout<<("!array_symbol, add variable for \n")<< vref->unparseToString()<<std::endl;
               }
               lower_exp= NULL; 
               length_exp= NULL; 
               lower_exp = (SgExpression*)($3);
               length_exp = (SgExpression*)($5);
               assert (array_symbol != NULL);
               SgType* t = array_symbol->get_type();
               bool isPointer= (isSgPointerType(t) != NULL );
               bool isArray= (isSgArrayType(t) != NULL);
               if (!isPointer && ! isArray )
               {
                 std::cerr<<"Error. ompparser.yy expects a pointer or array type."<<std::endl;
                 std::cerr<<"while seeing "<<t->class_name()<<std::endl;
               }
               assert (lower_exp && length_exp);
               ompattribute->array_dimensions[array_symbol].push_back( std::make_pair (lower_exp, length_exp));
             }  
            | postfix_expr PLUSPLUS {
                  current_exp = SageBuilder::buildPlusPlusOp(
                    (SgExpression*)($1),
                    SgUnaryOp::postfix
                  ); 
                  $$ = current_exp; 
                }
             | postfix_expr MINUSMINUS {
                  current_exp = SageBuilder::buildMinusMinusOp(
                    (SgExpression*)($1),
                    SgUnaryOp::postfix
                  ); 
                  $$ = current_exp; 
             }
            ;

/* ----------------------end for parsing expressions ------------------*/

/*  in C
variable-list : identifier
              | variable-list , identifier 
*/

/* in C++ (we use the C++ version) */ 
variable_list : ID_EXPRESSION {
              if (is_complex_clause) {
                addComplexVar((const char*)$1);
              }
              else {
                if (!addVar((const char*)$1)) {
                    YYABORT;
                };
              }
            }
              | variable_list ',' ID_EXPRESSION {
              if (is_complex_clause) {
                addComplexVar((const char*)$3);
              }
              else {
                if (!addVar((const char*)$3)) {
                    YYABORT;
                };
              }
            }


//if (!addVar((const char*)$3)) YYABORT; }
              ;

/*  depend( array1[i][k], array2[p][l]), real array references in the list  */
variable_exp_list : postfix_expr { 
                 if (!arraySection) // regular array or scalar references: we add the entire array reference to the variable list
                   if (!addVarExp((SgExpression*)$1)) YYABORT; 
                 array_symbol = NULL; //reset array symbol when done.   
               }
              | variable_exp_list ',' postfix_expr 
                { 
                 if (!arraySection)
                    if (!addVarExp((SgExpression*)$3)) YYABORT; 
                }
              ;


/* map (array[lower:length][lower:length])  , not array references, but array section notations */ 
map_variable_list : id_expression_opt_dimension
              | map_variable_list ',' id_expression_opt_dimension
              ;
/* mapped variables may have optional dimension information */
id_expression_opt_dimension: ID_EXPRESSION { if (!addVar((const char*)$1)) YYABORT; } dimension_field_optseq
                           ;

/* Parse optional dimension information associated with map(a[0:n][0:m]) Liao 1/22/2013 */
dimension_field_optseq: /* empty */
                      | dimension_field_seq
                      ;
/* sequence of dimension fields */
dimension_field_seq : dimension_field
                    | dimension_field_seq dimension_field
                    ;

dimension_field: '[' expression {lower_exp = current_exp; } 
                 ':' expression { length_exp = current_exp;
                      assert (array_symbol != NULL);
                      SgType* t = array_symbol->get_type();
                      bool isPointer= (isSgPointerType(t) != NULL );
                      bool isArray= (isSgArrayType(t) != NULL);
                      if (!isPointer && ! isArray )
                      {
                        std::cerr<<"Error. ompparser.yy expects a pointer or array type."<<std::endl;
                        std::cerr<<"while seeing "<<t->class_name()<<std::endl;
                      }
                      ompattribute->array_dimensions[array_symbol].push_back( std::make_pair (lower_exp, length_exp));
                      } 
                  ']'
               ;
/* commenting out experimental stuff
Optional data distribution clause: dist_data(dim1_policy, dim2_policy, dim3_policy)
mixed keyword or variable parsing is tricky TODO 
one or more dimensions, each has a policy
reset current_exp to avoid leaving stale values
Optional (exp) for some policy                   
id_expression_opt_dimension: ID_EXPRESSION { if (!addVar((const char*)$1)) YYABORT; } dimension_field_optseq id_expression_opt_dist_data
                           ;
id_expression_opt_dist_data: empty 
                           | DIST_DATA '(' dist_policy_seq ')'
                           ;
dist_policy_seq: dist_policy_per_dim
               | dist_policy_seq ',' dist_policy_per_dim
               ;
dist_policy_per_dim: DUPLICATE  { ompattribute->appendDistDataPolicy(array_symbol, e_duplicate, NULL); }
                   | BLOCK dist_size_opt { ompattribute->appendDistDataPolicy(array_symbol, e_block, current_exp );  current_exp = NULL;}
                   | CYCLIC dist_size_opt { ompattribute->appendDistDataPolicy(array_symbol, e_cyclic, current_exp ); current_exp = NULL;}
                   ;
dist_size_opt: empty {current_exp = NULL;}
             | '(' expression ')'
             ;
*/

%%
int yyerror(const char *s) {
    SgLocatedNode* lnode = isSgLocatedNode(gNode);
    assert (lnode);
    printf("Error when parsing pragma:\n\t %s \n\t associated with node at line %d\n", orig_str, lnode->get_file_info()->get_line()); 
    printf(" %s!\n", s);
    assert(0);
    return 0; // we want to the program to stop on error
}


OmpAttribute* getParsedDirective() {
    return ompattribute;
}

void omp_parser_init(SgNode* aNode, const char* str) {
    orig_str = str;  
    omp_lexer_init(str);
    gNode = aNode;
}

static bool addVar(const char* var)  {
    array_symbol = ompattribute->addVariable(omptype,var);
    return true;
}

static bool addComplexVar(const char* var)  {
    array_symbol = ompattribute->addComplexClauseParametersVariable(omptype,var);
    return true;
}

static bool addVarExp(SgExpression* exp)  { // new interface to add variables, supporting array reference expressions
    array_symbol = ompattribute->addVariable(omptype,exp);
    return true;
}


// The ROSE's string-based AST construction is not stable,
// pass real expressions as SgExpression, Liao
static bool addExpression(const char* expr) {
    // ompattribute->addExpression(omptype,std::string(expr),NULL);
    // std::cout<<"debug: current expression is:"<<current_exp->unparseToString()<<std::endl;
    assert (current_exp != NULL);
    ompattribute->addExpression(omptype,std::string(expr),current_exp);
    return true;
}

static bool addUserDefinedParameter(const char* expr) {
    assert (current_exp != NULL);
    ompattribute->addUserDefinedParameter(omptype,std::string(expr), current_exp);
    return true;
}

