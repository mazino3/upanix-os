. ${UPANIX_HOME}/setup.sh $@

# -D_GLIBCXX_PROFILE

export CXX="$COMPILER"
export CXXFLAGS="$CPP_FLAGS"

> depend.d
for i in `ls *.cpp *.cc 2> /dev/null`
do
	obj=${i%.*}.o
	$CXX ${INCLUDE} -M $i >> depend.d
	echo "\t@echo \"compiling $i...\"" >> depend.d
	echo "\t@$CXX ${CXXFLAGS} ${INCLUDE} $i -o ${obj}" >> depend.d
done

export CC="$C_COMPILER"
export CFLAGS="$C_FLAGS"

for i in `ls *.c 2> /dev/null`
do
	obj=${i%.*}.o
	$CC ${INCLUDE} -M $i >> depend.d
	echo "\t@echo \"compiling $i...\"" >> depend.d
	echo "\t@$CC ${CFLAGS} ${INCLUDE} $i -o ${obj}" >> depend.d
done
