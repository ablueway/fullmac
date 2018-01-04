make clean
make
make rlib
rm Makefile
mv Makefile.release Makefile
make clean
rm -rf ./host/ap
rm -rf ./host/core
rm -rf ./host/mac

