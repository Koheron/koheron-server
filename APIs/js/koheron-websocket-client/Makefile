# (c) Koheron

all: init build

init:
	npm install
	mkdir -p lib/

build: init
	gulp compile-lib-js

tests: build
	gulp compile-tests-js
	node tests/tests.js

clean:
	rm -rf lib/