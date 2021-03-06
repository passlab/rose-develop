include $(top_srcdir)/config/Makefile.for.ROSE.includes.and.libs

EXTRA_DIST = 

bin_PROGRAMS = typeforge
noinst_PROGRAMS = 

CLEANFILES = 

typeforge_YFLAGS = -p ltl_ # yacc flags for specification parser (not used yet)
typeforge_CXXFLAGS = -Wall -O3 -march=native -ftree-vectorize
typeforge_DEPENDENCIES=

AM_LDFLAGS = $(ROSE_LIBS) $(LIBS_WITH_RPATH)
AM_CPPFLAGS = $(ROSE_INCLUDES) -I$(top_srcdir)/projects/typeforge/src -g -rdynamic

typeforge_LDADD = $(LIBS_WITH_RPATH) $(ROSE_LIBS)
typeforge__CXXFLAGS = -Wall -O3 -march=native -ftree-vectorize
typeforge_SOURCES = typeforge.C CommandLineOptions.C CastStats.C CastTransformer.C TFTypeTransformer.C CastGraphVis.C CppStdUtilities.C TFTransformation.C SpecFrontEnd.C ToolConfig.cpp CommandList.C Analysis.C TFHandles.C TFToolConfig.C

CHECK_DEFAULT=check-typeforge

check-local: $(CHECK_DEFAULT)

CHECK_TRACE_OPTION=

check-typeforge: typeforge
	@echo ================================================================
	@echo RUNNING TYPEFORGE TESTS
	@echo ================================================================
	rm -f typegraph.dot craft_initial.json rose_casttest3.C
#	./typeforge -DSKIP_ROSE_BUILTIN_DECLARATIONS --dot-type-graph $(srcdir)/tests/casttest1.C 
#	dot -Tpdf typegraph.dot -o type-graph.pdf
	./typeforge --spec-file=$(srcdir)/tests/casttest3.tf ${CHECK_TRACE_OPTION} $(srcdir)/tests/casttest3.C --compile
	g++ -Wall rose_casttest3.C
	./typeforge --spec-file=$(srcdir)/tests/type_replace_test1.tf ${CHECK_TRACE_OPTION} $(srcdir)/tests/type_replace_test1.C
	./typeforge --spec-file=$(srcdir)/tests/basetype_replace_test1.tf ${CHECK_TRACE_OPTION} $(srcdir)/tests/basetype_replace_test1.C
	./typeforge --spec-file=$(srcdir)/tests/json_replace_test.json ${CHECK_TRACE_OPTION} $(srcdir)/tests/json_replace_test.C 
	./typeforge --spec-file=$(srcdir)/tests/json_out_test.json ${CHECK_TRACE_OPTION} $(srcdir)/tests/json_out_test.C --compile
	./typeforge --spec-file=craft_initial.json ${CHECK_TRACE_OPTION} $(srcdir)/tests/json_out_test.C --compile
	./typeforge --spec-file=$(srcdir)/tests/ad_transformation_test.tf $(srcdir)/tests/casttest1.C
	./typeforge --spec-file=$(srcdir)/tests/ad_transformation_test.tf $(srcdir)/tests/ad_compound_test.C
	./typeforge --spec-file=$(srcdir)/tests/ad_transformation_test.tf $(srcdir)/tests/ad_array_test.C
	./typeforge --spec-file=$(srcdir)/tests/ad_transformation_test.tf $(srcdir)/tests/ad_global_test.C
	./typeforge --spec-file=$(srcdir)/tests/ad_include_pragma.json $(srcdir)/tests/include_pragma_test1.C
	./typeforge --spec-file=$(srcdir)/tests/ad_include_pragma.json $(srcdir)/tests/include_pragma_test2.C
	./typeforge --plugin=$(srcdir)/tests/ad_all.json $(srcdir)/tests/ad_all.C
	./typeforge --plugin=$(srcdir)/tests/tf_file_test.tf $(srcdir)/tests/tf_file_test.C 
	./typeforge --plugin=$(srcdir)/tests/set_out_test.json --typeforge-out=set.json $(srcdir)/tests/setTest.C
	./typeforge --plugin=set.json ${CHECK_TRACE_OPTION} $(srcdir)/tests/setTest.C
	./typeforge --plugin=$(srcdir)/tests/var_set_change.json ${CHECK_TRACE_OPTION} $(srcdir)/tests/setTest.C
	rm -f a.out *.json

docs:
	cd "$(srcdir)" && doxygen

clean-local:
	rm craft_initial.json 
	rm -f *.dot
	rm -f *.ps
	rm -f *.jpg
	rm -f rose_*.C
	rm -f *.ti

distclean-local: clean
	rm -f *.tgz
	rm -f *~
	rm -rf ../docs/doxygen
