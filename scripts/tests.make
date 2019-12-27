
TEST_ONLY_RECIPES := \
	lib_core_tests_only

TEST_RECIPES := \
	lib_core_tests

.PHONY: $(TEST_ONLY_RECIPES) tests_only $(TEST_RECIPES) tests clean_tests

lib_core_tests_only:
	@${MAKE} --no-print-directory -C lib/core/test -f Makefile

tests_only: $(TEST_ONLY_RECIPES)

lib_core_tests: | lib_core
	@${MAKE} --no-print-directory -C lib/core/test -f Makefile

tests: $(TEST_RECIPES)

clean_tests:
	@${MAKE} --no-print-directory -C lib/core/test -f Makefile clean

clean:: clean_tests
