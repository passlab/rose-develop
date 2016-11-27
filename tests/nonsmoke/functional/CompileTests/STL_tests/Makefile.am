include $(top_srcdir)/config/Makefile.for.ROSE.includes.and.libs

TEST_TRANSLATOR=$(top_builddir)/tests/nonsmoke/functional/testTranslator

# DQ (11/5/2016): Adding support for address sanitizer (borrowing the VALGRIND variable to support 
# this, this implementation needs review).  It might be better to prepend the information to the 
# CC and CXX variables. I think that the purify support uses this approach.
if ROSE_USE_SANITIZER
VALGRIND = $(NON_RTH_ADDRESS_SANITIZER_OPTIONS)
else
VALGRIND =
endif

$(TEST_TRANSLATOR):
	$(MAKE) -C $(top_builddir)/tests/nonsmoke/functional testTranslator

if ROSE_USING_GCC_VERSION_LATER_4_8
conditional-check-local: $(TEST_TRANSLATOR)
	$(VALGRIND) TOOL2=$(TEST_TRANSLATOR) TOOL1=$(CXX)  $(top_srcdir)/tests/nonsmoke/functional/CompileTests/STL_tests/stl-eval.sh
else
conditional-check-local:
        @echo "ROSE/tests/nonsmoke/functional/CompileTests/STL_tests: make check rule skipped (not using g++ 4.8+ compiler)."
endif

check-local: conditional-check-local
	@echo "*******************************************************************************************************************"
	@echo "****** ROSE/tests/nonsmoke/functional/CompileTests/STL_tests: make check rule complete (terminated normally) ******"
	@echo "*******************************************************************************************************************"

check-no-cleanup: $(TEST_TRANSLATOR)
	TOOL2=$(TEST_TRANSLATOR) TOOL1=$(CXX)  $(top_srcdir)/tests/nonsmoke/functional/CompileTests/STL_tests/stl-eval.sh no-cleanup