# (c) Koheron

all: init build

init:
	npm install
	mkdir -p lib/

build: init
	gulp compile-lib-js
	gulp compile-tests-js

tests: build
	node tests/tests.js

clean:
	rm -rf lib/