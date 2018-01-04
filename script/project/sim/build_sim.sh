cd ../../..
perl ./script/set_chip.pl 6030P
perl ./script/modify.pl
make clean
make
cd ./script/project/sim
