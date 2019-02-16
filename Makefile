BUILDDIR=build
DXCDIR=${HOME}/DirectXShaderCompiler
SPIRVDIR=${HOME}/SPIRV-Cross
INSTALLDIR=/mnt/c/Users/a2k00/Documents/hlsl-preview/media/wasm

CFLAGS  = -s ENVIRONMENT=web
CFLAGS += -s MODULARIZE=1
CFLAGS += -s WASM=1
#CFLAGS += -s TOTAL_MEMORY=256MB
CFLAGS += -s ALLOW_MEMORY_GROWTH=1
CFLAGS += --bind

DEBUG_CFLAGS=-g -s DEMANGLE_SUPPORT=1 -O0 -s ASSERTIONS=1 -s SAFE_HEAP=1 -s DISABLE_EXCEPTION_CATCHING=0

RELEASE_CFLAGS=-Oz -s DISABLE_EXCEPTION_CATCHING=0

#CFLAGS += ${DEBUG_CFLAGS}
CFLAGS += ${RELEASE_CFLAGS}

DXC_LIBS=-L${DXCDIR}/build/lib -ldxcompiler
DXC_INCLUDES=-I${DXCDIR}/include
DXC_FLAGS=-s EXPORT_NAME=DXC
#-s FILESYSTEM=1 -s FORCE_FILESYSTEM=1

SPIRV_INCLUDES=-I${SPIRVDIR} -I${SPIRVDIR}/include
SPIRV_LIBS=-L${SPIRVDIR}/lib -L${SPIRVDIR}/build -lspirv-cross-reflect -lspirv-cross-glsl -lspirv-cross-core -lspirv-cross-util
SPIRV_FLAGS=-s EXPORT_NAME=SPIRV


${BUILDDIR}: ${BUILDDIR}
	mkdir $<

${DXCDIR}:
	git clone https://github.com/Microsoft/DirectXShaderCompiler.git $<

${DXCDIR}/build: ${DXCDIR}
	cd $<
	mkdir build
	cd build
	emcmake cmake -GNinja -DCMAKE_BUILD_TYPE=Release `cat ../utils/cmake-predefined-config-params` ..

${SPIRVDIR}:
	git clone https://github.com/KhronosGroup/SPIRV-Cross.git $<

${SPIRVDIR}/build: ${SPIRVDIR}
	cd $<
	mkdir build
	cd build
	emcmake cmake -DCMAKE_BUILD_TYPE=Release ..

${DXCDIR}/build/lib/libdxcompiler.so:
	cd ${DXCDIR}/build && ninja libdxcompiler.so

.PHONY: dxc

${BUILDDIR}/dxcompiler.js: dxcompiler.cpp Makefile ${DXCDIR}/build/lib/libdxcompiler.so
	em++ $< -o $@ ${CFLAGS} ${DXC_INCLUDES} ${DXC_LIBS} ${DXC_FLAGS}

${BUILDDIR}/spirv.js: spirv.cpp Makefile
	em++ $< -o $@ ${CFLAGS} ${SPIRV_INCLUDES} ${SPIRV_LIBS} ${SPIRV_FLAGS}

${BUILDDIR}/dxcompiler.wasm: ${BUILDDIR}/dxcompiler.js

${BUILDDIR}/spirv.wasm: ${BUILDDIR}/spirv.js

.PHONY: all

all: ${BUILDDIR} ${BUILDDIR}/dxcompiler.js ${BUILDDIR}/dxcompiler.wasm ${BUILDDIR}/spirv.js ${BUILDDIR}/spirv.wasm

run: all
	node main.js test.hlsl

install: ${BUILDDIR}/dxcompiler.js ${BUILDDIR}/dxcompiler.wasm ${BUILDDIR}/spirv.js ${BUILDDIR}/spirv.wasm
	cp $^ ${INSTALLDIR}

clean:
	rm -f ${BUILDDIR}/dxcompiler.js ${BUILDDIR}/dxcompiler.wasm ${BUILDDIR}/spirv.js ${BUILDDIR}/spirv.wasm

.DEFAULT_GOAL := all

