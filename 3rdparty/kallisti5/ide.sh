#!/bin/bash

if [[ $# -ne 1 ]]; then
	echo "Configures Haiku for clangd / an IDE"
	echo "Usage: $0 <TOP_HAIKU_DIRECTORY>"
	echo "  TOP_HAIKU_DIRECTORY - Top Haiku source code directory"
	exit 1;
fi

export ARCH="x86_64"
export GCC_VER="13.3.0"

echo "Deploying latest clangd config and compile_commands..."
export SRC_TOP=$(pwd $1)

# Cleanup anything existing
rm $SRC_TOP/compile_commands.json
rm -rf $SRC_TOP/.cache

curl https://s3.us-east-1.wasabisys.com/haiku-extra/compile_commands_$ARCH.json -o $SRC_TOP/compile_commands.json
# Update HAIKU_SRC_TOP to our actual SRC_TOP.  I tried "." here, but clangd doesn't like it
sed -i "s@%%HAIKU_SRC_TOP%%@$SRC_TOP@g" $SRC_TOP/compile_commands.json
# I think this helps clangd understand our cross-compiler
echo "CompileFlags:" > $SRC_TOP/.clangd
echo "  Compiler: \"generated/cross-tools-$ARCH/bin/$ARCH-unknown-haiku-gcc\"" >> $SRC_TOP/.clangd
echo "  Add: [-I./generated/cross-tools-$ARCH/lib/gcc/$ARCH-unknown-haiku/$GCC_VER/include]" >> $SRC_TOP/.clangd

echo "========================================================================================="
echo "clangd has been configured for the haiku source code."
echo "========================================================================================="
echo ""
echo "NOTE: Generated headers will not be available (float.h, etc) until you compile Haiku and"
echo "symlink your specific generated directory into the base path!"
echo "   example: cd $SRC_TOP; ln -s generated.$ARCH generated"
