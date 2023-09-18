CL = $(shell getconf LEVEL1_DCACHE_LINESIZE)

CFLAGS = -fno-omit-frame-pointer -std=c++23 -Wall -Wextra -Wshadow -Wno-unused-result -Wno-sign-compare -Wno-char-subscripts -pedantic -DCACHE_LINE=$(CL) -march=native		

INCLUDE = -isystem bit_vector/

.DEFAULT: comp

comp: final.cpp
	g++ $(CFLAGS) $(INCLUDE) -DNDEBUG -O3 -o final final.cpp
