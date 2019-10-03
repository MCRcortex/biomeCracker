#include <iostream>
#include <unistd.h>
#include <vector>
#include <wait.h>
#include <cmath>
	#include "generationByCubitect/generator.hpp"
	#include "generationByCubitect/finders.hpp"

#include <mutex>

std::mutex mu;
#define PROCESSES 1

uint64_t baseSeed;
Pos pos;
void doStuff(uint64_t process_id) {
    initBiomes();
    LayerStack g = setupGenerator(MC_1_14);
	
    int *map = allocCache(&g.layers[g.layerNum - 1], 1, 1);
	
    for (uint64_t seed = (baseSeed+(process_id<<48));seed < (baseSeed+((uint64_t)0xFFFF<<48)); seed+=((uint64_t)PROCESSES<<48)) {

        applySeed(&g, (int64_t) seed);

        genArea(&g.layers[g.layerNum - 1], map, pos.x, pos.z, 1, 1);
        if (map[0] == 38) {
			
			std::cout << "gat one: "<<seed<<"\n";
        }
    }
    free(map);
    freeGenerator(g);
    exit(0);

}

int main() {
	baseSeed=0;
	pos = {0, 0};
    doStuff(0);
}
