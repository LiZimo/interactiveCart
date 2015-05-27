./swap_test.sh -nn 0 -rk2 0 -v 1 -swap 1 > bilerp_rk4_tall.txt
./swap_test.sh -nn 0 -rk2 1 -v 1 -swap 1 > bilerp_rk2_tall.txt
./swap_test.sh -nn 1 -rk2 0 -v 1 -swap 1 > nn_rk4_tall.txt 
./swap_test.sh -nn 1 -rk2 1 -v 1 -swap 1 > nn_rk2_tall.txt

./swap_test.sh -nn 0 -rk2 0 -v 1 -swap 0 > bilerp_rk4_fat.txt
./swap_test.sh -nn 0 -rk2 1 -v 1 -swap 0 > bilerp_rk2_fat.txt
./swap_test.sh -nn 1 -rk2 0 -v 1 -swap 0 > nn_rk4_fat.txt 
./swap_test.sh -nn 1 -rk2 1 -v 1 -swap 0 > nn_rk2_fat.txt
