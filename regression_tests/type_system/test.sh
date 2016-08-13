#!/bin/bash
COUNT_LIST=${*:-10 25 100 500}
TEST_LIST=(
    'current_struct'
    'struct_fields'
    'typelist'
    'typelist_dynamic'
    'rtti_dynamic_call'
    'variant'
    'json_variant_stringify'
    'json_variant_stringify_and_parse'
    'json_variant_stringify_and_parse_and_test'
    'sherlock_memory'
    'sherlock_file'
    'storage_null_persister'
    'storage_simple_persister'
    'storage_sherlock_persister'
    'storage_with_subscriber'
)

CPLUSPLUS=${CPLUSPLUS:-g++}
CPPFLAGS=${CPPFLAGS:- -std=c++11 -ftemplate-backtrace-limit=0  -ftemplate-depth=10000}
LDFLAGS=${LDFLAGS:- -pthread}

mkdir -p .current

# NOTE(dkorolev): The '.current/typelist_impl' test is the same as '.current/typelist', and it's omitted for now.

for c in ${COUNT_LIST[@]}; do
	./gen_test_data.sh $c
	for t in "${TEST_LIST[@]}"; do
		echo -e -n "\033[1m\033[39mBuilding\033[0m "
		echo -e -n "\033[36m"
		echo $t
		echo -e -n "\033[1m\033[91m"
		TIMEFORMAT='Compile: %R seconds'
    time $CPLUSPLUS $CPPFLAGS -c -o .current/$t.o $t.cc >/dev/null
		TIMEFORMAT='Link:    %R seconds'
    time $CPLUSPLUS $LDFLAGS -o .current/$t .current/$t.o >/dev/null
		echo -e -n "\033[1m\033[39mRunning\033[0m: "
		./.current/$t >/dev/null
		if [[ $? -eq 0 ]]; then
			echo -e "\033[32mPASSED\033[39m"
		else
			echo -e "\033[31mFAILED\033[39m"
		fi
	done
	echo
done
