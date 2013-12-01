# This file is part of the "libsmatrix" project
#   (c) 2011-2013 Paul Asmuth <paul@paulasmuth.com>
#
# Licensed under the MIT License (the "License"); you may not use this
# file except in compliance with the License. You may obtain a copy of
# the License at: http://opensource.org/licenses/MIT

default: all

test: clean all test_java

test_java:
	javac -classpath ./src:./src/java src/java/test/TestSparseMatrix.java
	java -classpath ./src:./src/java:./src/java/test TestSparseMatrix

build_maven:
	mvn package

publish_maven: build_maven
	mvn deploy

pom.xml:
	ln -s build/maven/pom.xml

.DEFAULT:
	cd src && $(MAKE) $@
