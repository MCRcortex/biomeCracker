#include "finders.hpp"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


Pos getStructurePos(const StructureConfig config, int64_t seed,
        const int regionX, const int regionZ)
{
    Pos pos;

    // set seed
    seed = regionX*341873128712 + regionZ*132897987541 + seed + config.seed;
    seed = (seed ^ 0x5deece66dLL);// & ((1LL << 48) - 1);

    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;

    if (config.properties & USE_POW2_RNG)
    {
        // Java RNG treats powers of 2 as a special case.
        pos.x = (config.chunkRange * (seed >> 17)) >> 31;

        seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
        pos.z = (config.chunkRange * (seed >> 17)) >> 31;
    }
    else
    {
        pos.x = (int)(seed >> 17) % config.chunkRange;

        seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
        pos.z = (int)(seed >> 17) % config.chunkRange;
    }

    pos.x = ((regionX*config.regionSize + pos.x) << 4) + 8;
    pos.z = ((regionZ*config.regionSize + pos.z) << 4) + 8;
    return pos;
}

Pos getStructureChunkInRegion(const StructureConfig config, int64_t seed,
        const int regionX, const int regionZ)
{
    /*
    // Vanilla like implementation.
    seed = regionX*341873128712 + regionZ*132897987541 + seed + structureSeed;
    setSeed(&(seed));

    Pos pos;
    pos.x = nextInt(&seed, 24);
    pos.z = nextInt(&seed, 24);
    */
    Pos pos;

    seed = regionX*341873128712 + regionZ*132897987541 + seed + config.seed;
    seed = (seed ^ 0x5deece66dLL);// & ((1LL << 48) - 1);

    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;

    if (config.properties & USE_POW2_RNG)
    {
        // Java RNG treats powers of 2 as a special case.
        pos.x = (config.chunkRange * (seed >> 17)) >> 31;

        seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
        pos.z = (config.chunkRange * (seed >> 17)) >> 31;
    }
    else
    {
        pos.x = (int)(seed >> 17) % config.chunkRange;

        seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
        pos.z = (int)(seed >> 17) % config.chunkRange;
    }

    return pos;
}


Pos getLargeStructurePos(StructureConfig config, int64_t seed,
        const int regionX, const int regionZ)
{
    Pos pos;

    //TODO: if (config.properties & USE_POW2_RNG)...

    // set seed
    seed = regionX*341873128712 + regionZ*132897987541 + seed + config.seed;
    seed = (seed ^ 0x5deece66dLL) & ((1LL << 48) - 1);

    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
    pos.x = (seed >> 17) % config.chunkRange;
    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
    pos.x += (seed >> 17) % config.chunkRange;

    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
    pos.z = (seed >> 17) % config.chunkRange;
    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
    pos.z += (seed >> 17) % config.chunkRange;

    pos.x = regionX*config.regionSize + (pos.x >> 1);
    pos.z = regionZ*config.regionSize + (pos.z >> 1);
    pos.x = pos.x*16 + 8;
    pos.z = pos.z*16 + 8;
    return pos;
}


Pos getLargeStructureChunkInRegion(StructureConfig config, int64_t seed,
        const int regionX, const int regionZ)
{
    Pos pos;

    //TODO: if (config.properties & USE_POW2_RNG)...

    // set seed
    seed = regionX*341873128712 + regionZ*132897987541 + seed + config.seed;
    seed = (seed ^ 0x5deece66dLL) & ((1LL << 48) - 1);

    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
    pos.x = (seed >> 17) % config.chunkRange;
    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
    pos.x += (seed >> 17) % config.chunkRange;

    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
    pos.z = (seed >> 17) % config.chunkRange;
    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
    pos.z += (seed >> 17) % config.chunkRange;

    pos.x >>= 1;
    pos.z >>= 1;

    return pos;
}


//==============================================================================
// Checking Biomes & Biome Helper Functions
//==============================================================================


int getBiomeAtPos(const LayerStack g, const Pos pos)
{
    int *map = allocCache(&g.layers[g.layerNum-1], 1, 1);
    genArea(&g.layers[g.layerNum-1], map, pos.x, pos.z, 1, 1);
    int biomeID = map[0];
    free(map);
    return biomeID;
}

Pos findBiomePosition(
        const int mcversion,
        const LayerStack g,
        int *cache,
        const int centerX,
        const int centerZ,
        const int range,
        const int *isValid,
        int64_t *seed,
        int *passes
        )
{
    int x1 = (centerX-range) >> 2;
    int z1 = (centerZ-range) >> 2;
    int x2 = (centerX+range) >> 2;
    int z2 = (centerZ+range) >> 2;
    int width  = x2 - x1 + 1;
    int height = z2 - z1 + 1;
    int *map;
    int i, j, found;

    Layer *layer = &g.layers[L_RIVER_MIX_4];
    Pos out;

    if (layer->scale != 4)
    {
        printf("WARN findBiomePosition: The generator has unexpected scale %d at layer %d.\n",
                layer->scale, L_RIVER_MIX_4);
    }

    map = cache ? cache : allocCache(layer, width, height);

    genArea(layer, map, x1, z1, width, height);

    out.x = centerX;
    out.z = centerZ;
    found = 0;

    if (mcversion >= MC_1_13)
    {
        for (i = 0, j = 2; i < width*height; i++)
        {
            if (!isValid[map[i] & 0xff]) continue;
            if ((found == 0 || nextInt(seed, j++) == 0))
            {
                out.x = (x1 + i%width) << 2;
                out.z = (z1 + i/width) << 2;
                found = 1;
            }
        }
        found = j - 2;
    }
    else
    {
        for (i = 0; i < width*height; i++)
        {
            if (isValid[map[i] & 0xff] &&
                (found == 0 || nextInt(seed, found + 1) == 0))
            {
                out.x = (x1 + i%width) << 2;
                out.z = (z1 + i/width) << 2;
                ++found;
            }
        }
    }


    if (cache == NULL)
    {
        free(map);
    }

    if (passes != NULL)
    {
        *passes = found;
    }

    return out;
}


int areBiomesViable(
        const LayerStack    g,
        int *               cache,
        const int           posX,
        const int           posZ,
        const int           radius,
        const int *         isValid
        )
{
    int x1 = (posX - radius) >> 2;
    int z1 = (posZ - radius) >> 2;
    int x2 = (posX + radius) >> 2;
    int z2 = (posZ + radius) >> 2;
    int width = x2 - x1 + 1;
    int height = z2 - z1 + 1;
    int i;
    int *map;

    Layer *layer = &g.layers[L_RIVER_MIX_4];

    if (layer->scale != 4)
    {
        printf("WARN areBiomesViable: The generator has unexpected scale %d at layer %d.\n",
                layer->scale, L_RIVER_MIX_4);
    }

    map = cache ? cache : allocCache(layer, width, height);
    genArea(layer, map, x1, z1, width, height);

    for (i = 0; i < width*height; i++)
    {
        if (!isValid[ map[i] & 0xff ])
        {
            if (cache == NULL) free(map);
            return 0;
        }
    }

    if (cache == NULL) free(map);
    return 1;
}


int getBiomeRadius(
        const int *         map,
        const int           mapSide,
        const int *         biomes,
        const int           bnum,
        const int           ignoreMutations)
{
    int r, i, b;
    int blist[0x100];
    int mask = ignoreMutations ? 0x7f : 0xff;
    int radiusMax = mapSide / 2;

    if ((mapSide & 1) == 0)
    {
        printf("WARN getBiomeRadius: Side length of the square map should be an odd integer.\n");
    }

    memset(blist, 0, sizeof(blist));

    for (r = 1; r < radiusMax; r++)
    {
        for (i = radiusMax-r; i <= radiusMax+r; i++)
        {
            blist[ map[(radiusMax-r) * mapSide+ i]    & mask ] = 1;
            blist[ map[(radiusMax+r-1) * mapSide + i] & mask ] = 1;
            blist[ map[mapSide*i + (radiusMax-r)]     & mask ] = 1;
            blist[ map[mapSide*i + (radiusMax+r-1)]   & mask ] = 1;
        }

        for (b = 0; b < bnum && blist[biomes[b] & mask]; b++);
        if (b >= bnum)
        {
            break;
        }
    }

    return r != radiusMax ? r : -1;
}



//==============================================================================
// Finding Strongholds and Spawn
//==============================================================================


int* getValidStrongholdBiomes()
{
    static int validStrongholdBiomes[256];

    if (!validStrongholdBiomes[plains])
    {
        int id;
        for (id = 0; id < 256; id++)
        {
            if (biomeExists(id) && biomes[id].height > 0.0)
                validStrongholdBiomes[id] = 1;
        }
    }

    return validStrongholdBiomes;
}


int findStrongholds(const int mcversion, LayerStack *g, int *cache,
        Pos *locations, int64_t worldSeed, int maxSH, const int maxRadius)
{
    const int *validStrongholdBiomes = getValidStrongholdBiomes();
    int i, x, z;
    double distance;

    int currentRing = 0;
    int currentCount = 0;
    int perRing = 3;

    setSeed(&worldSeed);
    double angle = nextDouble(&worldSeed) * PI * 2.0;

    if (mcversion >= MC_1_9)
    {
        if (maxSH <= 0) maxSH = 128;

        for (i = 0; i < maxSH; i++)
        {
            distance = (4.0 * 32.0) + (6.0 * currentRing * 32.0) +
                (nextDouble(&worldSeed) - 0.5) * 32 * 2.5;

            if (maxRadius && distance*16 > maxRadius)
                return i;

            x = (int)round(cos(angle) * distance);
            z = (int)round(sin(angle) * distance);

            locations[i] = findBiomePosition(mcversion, *g, cache,
                    (x << 4) + 8, (z << 4) + 8, 112, validStrongholdBiomes,
                    &worldSeed, NULL);

            angle += 2 * PI / perRing;

            currentCount++;
            if (currentCount == perRing)
            {
                // Current ring is complete, move to next ring.
                currentRing++;
                currentCount = 0;
                perRing = perRing + 2*perRing/(currentRing+1);
                if (perRing > 128-i)
                    perRing = 128-i;
                angle = angle + nextDouble(&worldSeed) * PI * 2.0;
            }
        }
    }
    else
    {
        if (maxSH <= 0) maxSH = 3;

        for (i = 0; i < maxSH; i++)
        {
            distance = (1.25 + nextDouble(&worldSeed)) * 32.0;

            if (maxRadius && distance*16 > maxRadius)
                return i;

            x = (int)round(cos(angle) * distance);
            z = (int)round(sin(angle) * distance);

            locations[i] = findBiomePosition(mcversion, *g, cache,
                    (x << 4) + 8, (z << 4) + 8, 112, validStrongholdBiomes,
                    &worldSeed, NULL);

            angle += 2 * PI / 3.0;
        }
    }

    return maxSH;
}


static double getGrassProbability(int64_t seed, int biome, int x, int z)
{
    // TODO: Use ChunkGeneratorOverworld.generateHeightmap for better estimate.
    // TODO: Try to determine the actual probabilities and build a statistic.
    switch (biome)
    {
    case plains:            return 1.0;
    case extremeHills:      return 0.8; // height dependent
    case forest:            return 1.0;
    case taiga:             return 1.0;
    case swampland:         return 0.6; // height dependent
    case river:             return 0.2;
    case beach:             return 0.1;
    case forestHills:       return 1.0;
    case taigaHills:        return 1.0;
    case extremeHillsEdge:  return 1.0; // height dependent
    case jungle:            return 1.0;
    case jungleHills:       return 1.0;
    case jungleEdge:        return 1.0;
    case birchForest:       return 1.0;
    case birchForestHills:  return 1.0;
    case roofedForest:      return 0.9;
    case coldTaiga:         return 0.1; // below trees
    case coldTaigaHills:    return 0.1; // below trees
    case megaTaiga:         return 0.6;
    case megaTaigaHills:    return 0.6;
    case extremeHillsPlus:  return 0.2; // height dependent
    case savanna:           return 1.0;
    case savannaPlateau:    return 1.0;
    case mesaPlateau_F:     return 0.1; // height dependent
    case mesaPlateau:       return 0.1; // height dependent
    // NOTE: in rare circumstances you can get also get grass islands that are
    // completely ocean variants...
    default: return 0;
    }
}

static int canCoordinateBeSpawn(const int64_t seed, LayerStack *g, int *cache, Pos pos)
{
    int biome = getBiomeAtPos(*g, pos);
    return getGrassProbability(seed, biome, pos.x, pos.z) >= 0.5;
}

static int* getValidSpawnBiomes()
{
    static int isSpawnBiome[256];
    unsigned int i;

    if (!isSpawnBiome[biomesToSpawnIn[0]])
    {
        for (i = 0; i < sizeof(biomesToSpawnIn) / sizeof(int); i++)
        {
            isSpawnBiome[ biomesToSpawnIn[i] ] = 1;
        }
    }

    return isSpawnBiome;
}


Pos getSpawn(const int mcversion, LayerStack *g, int *cache, int64_t worldSeed)
{
    const int *isSpawnBiome = getValidSpawnBiomes();
    Pos spawn;
    int found;
    int i;

    setSeed(&worldSeed);
    spawn = findBiomePosition(mcversion, *g, cache, 0, 0, 256, isSpawnBiome,
            &worldSeed, &found);

    if (!found)
    {
        //printf("Unable to find spawn biome.\n");
        spawn.x = spawn.z = 8;
    }

    if (mcversion >= MC_1_13)
    {
        // TODO: The 1.13 section may need further checking!
        int n2 = 0;
        int n3 = 0;
        int n4 = 0;
        int n5 = -1;

        for (i = 0; i < 1024; i++)
        {
            if (n2 > -16 && n2 <= 16 && n3 > -16 && n3 <= 16)
            {
                int cx = ((spawn.x >> 4) + n2) << 4;
                int cz = ((spawn.z >> 4) + n3) << 4;

                for (int i2 = cx; i2 <= cx+15; i2++)
                {
                    for (int i3 = cz; i3 <= cz+15; i3++)
                    {
                        Pos pos = {i2, i3};
                        if (canCoordinateBeSpawn(worldSeed, g, cache, pos))
                        {
                            return pos;
                        }
                    }
                }
            }

            if (n2 == n3 || (n2 < 0 && n2 == - n3) || (n2 > 0 && n2 == 1 - n3))
            {
                int n7 = n4;
                n4 = - n5;
                n5 = n7;
            }
            n2 += n4;
            n3 += n5;
        }
    }
    else
    {
        for (i = 0; i < 1000 && !canCoordinateBeSpawn(worldSeed, g, cache, spawn); i++)
        {
            spawn.x += nextInt(&worldSeed, 64) - nextInt(&worldSeed, 64);
            spawn.z += nextInt(&worldSeed, 64) - nextInt(&worldSeed, 64);
        }
    }

    return spawn;
}


Pos estimateSpawn(const int mcversion, LayerStack *g, int *cache, int64_t worldSeed)
{
    const int *isSpawnBiome = getValidSpawnBiomes();
    Pos spawn;
    int found;

    setSeed(&worldSeed);
    spawn = findBiomePosition(mcversion, *g, cache, 0, 0, 256, isSpawnBiome,
            &worldSeed, &found);

    if (!found)
    {
        spawn.x = spawn.z = 8;
    }

    return spawn;
}



//==============================================================================
// Validating Structure Positions
//==============================================================================


int isViableFeaturePos(const int structureType, const LayerStack g, int *cache,
        const int blockX, const int blockZ)
{
    int *map = cache ? cache : allocCache(&g.layers[g.layerNum-1], 1, 1);
    genArea(&g.layers[g.layerNum-1], map, blockX, blockZ, 1, 1);
    int biomeID = map[0];
    if (!cache) free(map);

    switch(structureType)
    {
    case Desert_Pyramid:
        return biomeID == desert || biomeID == desertHills;
    case Igloo:
        return biomeID == icePlains || biomeID == coldTaiga;
    case Jungle_Pyramid:
        return biomeID == jungle || biomeID == jungleHills;
    case Swamp_Hut:
        return biomeID == swampland;
    case Ocean_Ruin:
    case Shipwreck:
        return isOceanic(biomeID);
    default:
        fprintf(stderr, "Structure type is not valid for the scattered feature biome check.\n");
        exit(1);
    }
}

int isViableVillagePos(const LayerStack g, int *cache,
        const int blockX, const int blockZ)
{
    static int isVillageBiome[0x100];

    if (!isVillageBiome[villageBiomeList[0]])
    {
        unsigned int i;
        for (i = 0; i < sizeof(villageBiomeList) / sizeof(int); i++)
        {
            isVillageBiome[ villageBiomeList[i] ] = 1;
        }
    }

    return areBiomesViable(g, cache, blockX, blockZ, 0, isVillageBiome);
}

int isViableOceanMonumentPos(const LayerStack g, int *cache,
        const int blockX, const int blockZ)
{
    static int isWaterBiome[0x100];
    static int isDeepOcean[0x100];

    if (!isWaterBiome[oceanMonumentBiomeList1[1]])
    {
        unsigned int i;
        for (i = 0; i < sizeof(oceanMonumentBiomeList1) / sizeof(int); i++)
        {
            isWaterBiome[ oceanMonumentBiomeList1[i] ] = 1;
        }

        for (i = 0; i < sizeof(oceanMonumentBiomeList2) / sizeof(int); i++)
        {
            isDeepOcean[ oceanMonumentBiomeList2[i] ] = 1;
        }
    }

    return areBiomesViable(g, cache, blockX, blockZ, 16, isDeepOcean) &&
            areBiomesViable(g, cache, blockX, blockZ, 29, isWaterBiome);
}

int isViableMansionPos(const LayerStack g, int *cache,
        const int blockX, const int blockZ)
{
    static int isMansionBiome[0x100];

    if (!isMansionBiome[mansionBiomeList[0]])
    {
        unsigned int i;
        for (i = 0; i < sizeof(mansionBiomeList) / sizeof(int); i++)
        {
            isMansionBiome[ mansionBiomeList[i] ] = 1;
        }
    }

    return areBiomesViable(g, cache, blockX, blockZ, 32, isMansionBiome);
}




//==============================================================================
// Finding Properties of Structures
//==============================================================================


int isZombieVillage(const int mcversion, const int64_t worldSeed,
        const int regionX, const int regionZ)
{
    Pos pos;
    int64_t seed = worldSeed;

    if (mcversion < MC_1_10)
    {
        printf("Warning: Zombie villages were only introduced in MC 1.10.\n");
    }

    // get the chunk position of the village
    seed = regionX*341873128712 + regionZ*132897987541 + seed + VILLAGE_CONFIG.seed;
    seed = (seed ^ 0x5deece66dLL);// & ((1LL << 48) - 1);

    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
    pos.x = (seed >> 17) % VILLAGE_CONFIG.chunkRange;

    seed = (seed * 0x5deece66dLL + 0xbLL) & 0xffffffffffff;
    pos.z = (seed >> 17) % VILLAGE_CONFIG.chunkRange;

    pos.x += regionX * VILLAGE_CONFIG.regionSize;
    pos.z += regionZ * VILLAGE_CONFIG.regionSize;

    // jump to the random number check that determines whether this is village
    // is zombie infested
    int64_t rnd = chunkGenerateRnd(worldSeed, pos.x , pos.z);
    // TODO: check for versions <= 1.11
    skipNextN(&rnd, mcversion >= MC_1_13 ? 10 : 11);

    return nextInt(&rnd, 50) == 0;
}


int isBabyZombieVillage(const int mcversion, const int64_t worldSeed,
        const int regionX, const int regionZ)
{
    if (!isZombieVillage(mcversion, worldSeed, regionX, regionZ))
        return 0;

    // Whether the zombie is a child or not is dependent on the world random
    // object which is not reset for villages. The last reset is instead
    // performed during the positioning of Mansions.
    int64_t rnd = worldSeed;
    rnd = regionX*341873128712 + regionZ*132897987541 + rnd + MANSION_CONFIG.seed;
    setSeed(&rnd);
    skipNextN(&rnd, 5);

    int isChild = nextFloat(&rnd) < 0.05;
    //int mountNearbyChicken = nextFloat(&rnd) < 0.05;
    //int spawnNewChicken = nextFloat(&rnd) < 0.05;

    return isChild;
}


int64_t getHouseList(const int64_t worldSeed, const int chunkX, const int chunkZ,
        int *out)
{
    int64_t rnd = chunkGenerateRnd(worldSeed, chunkX, chunkZ);
    skipNextN(&rnd, 1);

    out[HouseSmall] = nextInt(&rnd, 4 - 2 + 1) + 2;
    out[Church]     = nextInt(&rnd, 1 - 0 + 1) + 0;
    out[Library]    = nextInt(&rnd, 2 - 0 + 1) + 0;
    out[WoodHut]    = nextInt(&rnd, 5 - 2 + 1) + 2;
    out[Butcher]    = nextInt(&rnd, 2 - 0 + 1) + 0;
    out[FarmLarge]  = nextInt(&rnd, 4 - 1 + 1) + 1;
    out[FarmSmall]  = nextInt(&rnd, 4 - 2 + 1) + 2;
    out[Blacksmith] = nextInt(&rnd, 1 - 0 + 1) + 0;
    out[HouseLarge] = nextInt(&rnd, 3 - 0 + 1) + 0;

    return rnd;
}

//==============================================================================
// Seed Filters
//==============================================================================


int64_t filterAllTempCats(
        LayerStack *        g,
        int *               cache,
        const int64_t *     seedsIn,
        int64_t *           seedsOut,
        const int64_t       seedCnt,
        const int           centX,
        const int           centZ)
{
    /* We require all temperature categories, including the special variations
     * in order to get all main biomes. This gives 8 required values:
     * Oceanic, Warm, Lush, Cold, Freezing,
     * Special Warm, Special Lush, Special Cold
     * These categories generate at Layer 13: Edge, Special.
     *
     * Note: The scale at this layer is 1:1024 and each element can "leak" its
     * biome values up to 1024 blocks outwards into the negative coordinates
     * (due to the Zoom layers).
     *
     * The plan is to check if the 3x3 area contains all 8 temperature types.
     * For this, we can check even earlier at Layer 10: Add Island, that each of
     * the Warm, Cold and Freezing categories are present.
     */

    /* Edit:
     * All the biomes that are generated by a simple Cold climate can actually
     * be generated later on. So I have commented out the Cold requirements.
     */

    const int pX = centX-1, pZ = centZ-1;
    const int sX = 3, sZ = 3;
    int *map;

    Layer *lFilterSnow = &g->layers[L_ADD_SNOW_1024];
    Layer *lFilterSpecial = &g->layers[L_SPECIAL_1024];

    map = cache ? cache : allocCache(lFilterSpecial, sX, sZ);

    // Construct a dummy Edge,Special layer.
    Layer layerSpecial;
    setupLayer(1024, &layerSpecial, NULL, 3, NULL);

    int64_t sidx, hits, seed;
    int types[9];
    int specialCnt;
    int i, j;

    hits = 0;

    for (sidx = 0; sidx < seedCnt; sidx++)
    {
        seed = seedsIn[sidx];

        /***  Pre-Generation Checks  ***/

        // We require at least 3 special temperature categories which can be
        // tested for without going through the previous layers. (We'll get
        // false positives due to Oceans, but this works fine to rule out some
        // seeds early on.)
        setWorldSeed(&layerSpecial, seed);
        specialCnt = 0;
        for (i = 0; i < sX; i++)
        {
            for (j = 0; j < sZ; j++)
            {
                setChunkSeed(&layerSpecial, (int64_t)(i+pX), (int64_t)(j+pZ));
                if (mcNextInt(&layerSpecial, 13) == 0)
                    specialCnt++;
            }
        }

        if (specialCnt < 3)
        {
            continue;
        }

        /***  Cold/Warm Check  ***/

        // Continue by checking if enough cold and warm categories are present.
        setWorldSeed(lFilterSnow, seed);
        genArea(lFilterSnow, map, pX,pZ, sX,sZ);

        memset(types, 0, sizeof(types));
        for (i = 0; i < sX*sZ; i++)
            types[map[i]]++;

        // 1xOcean needs to be present
        // 4xWarm need to turn into Warm, Lush, Special Warm and Special Lush
        // 1xFreezing that needs to stay Freezing
        // 3x(Cold + Freezing) for Cold, Special Cold and Freezing
        if ( types[Ocean] < 1 || types[Warm] < 4 || types[Freezing] < 1 ||
            types[Cold]+types[Freezing] < 2)
        {
            continue;
        }

        /***  Complete Temperature Category Check  ***/

        // Check that all temperature variants are present.
        setWorldSeed(lFilterSpecial, seed);
        genArea(lFilterSpecial, map, pX,pZ, sX,sZ);

        memset(types, 0, sizeof(types));
        for (i = 0; i < sX*sZ; i++)
            types[ map[i] > 4 ? (map[i]&0xf) + 4 : map[i] ]++;

        if ( types[Ocean] < 1  || types[Warm] < 1     || types[Lush] < 1 ||
            /*types[Cold] < 1   ||*/ types[Freezing] < 1 ||
            types[Warm+4] < 1 || types[Lush+4] < 1   || types[Cold+4] < 1)
        {
            continue;
        }

        /*
        for (i = 0; i < sX*sZ; i++)
        {
            printf("%c%d ", " s"[cache[i] > 4], cache[i]&0xf);
            if (i % sX == sX-1) printf("\n");
        }
        printf("\n");*/

        // Save the candidate.
        seedsOut[hits] = seed;
        hits++;
    }

    if (cache == NULL) free(map);
    return hits;
}


const int majorBiomes[] = {
        ocean, plains, desert, extremeHills, forest, taiga, swampland,
        icePlains, mushroomIsland, jungle, deepOcean, birchForest, roofedForest,
        coldTaiga, megaTaiga, savanna, mesaPlateau_F, mesaPlateau
};

int64_t filterAllMajorBiomes(
        LayerStack *        g,
        int *               cache,
        const int64_t *     seedsIn,
        int64_t *           seedsOut,
        const int64_t       seedCnt,
        const int           pX,
        const int           pZ,
        const unsigned int  sX,
        const unsigned int  sZ)
{
    Layer *lFilterMushroom = &g->layers[L_ADD_MUSHROOM_ISLAND_256];
    Layer *lFilterBiomes = &g->layers[L_BIOME_256];

    int *map;
    int64_t sidx, seed, hits;
    unsigned int i, id, hasAll;

    int types[BIOME_NUM];

    map = cache ? cache : allocCache(lFilterBiomes, sX, sZ);

    hits = 0;

    for (sidx = 0; sidx < seedCnt; sidx++)
    {
        /* We can use the Mushroom layer both to check for mushroomIsland biomes
         * and to make sure all temperature categories are present in the area.
         */
        seed = seedsIn[sidx];
        setWorldSeed(lFilterMushroom, seed);
        genArea(lFilterMushroom, map, pX,pZ, sX,sZ);

        memset(types, 0, sizeof(types));
        for (i = 0; i < sX*sZ; i++)
        {
            id = map[i];
            if (id >= BIOME_NUM) id = (id & 0xf) + 4;
            types[id]++;
        }

        if ( types[Ocean] < 1  || types[Warm] < 1     || types[Lush] < 1 ||
         /* types[Cold] < 1   || */ types[Freezing] < 1 ||
            types[Warm+4] < 1 || types[Lush+4] < 1   || types[Cold+4] < 1 ||
            types[mushroomIsland] < 1)
        {
            continue;
        }

        /***  Find all major biomes  ***/

        setWorldSeed(lFilterBiomes, seed);
        genArea(lFilterBiomes, map, pX,pZ, sX,sZ);

        memset(types, 0, sizeof(types));
        for (i = 0; i < sX*sZ; i++)
        {
            types[map[i]]++;
        }

        hasAll = 1;
        for (i = 0; i < sizeof(majorBiomes) / sizeof(*majorBiomes); i++)
        {
            // plains, taiga and deepOcean can be generated in later layers.
            // Also small islands of Forests can be generated in deepOcean
            // biomes, but we are going to ignore those.
            if (majorBiomes[i] == plains ||
               majorBiomes[i] == taiga ||
               majorBiomes[i] == deepOcean)
            {
                continue;
            }

            if (types[majorBiomes[i]] < 1)
            {
                hasAll = 0;
                break;
            }
        }
        if (!hasAll)
        {
            continue;
        }

        seedsOut[hits] = seed;
        hits++;
    }

    if (cache == NULL) free(map);
    return hits;
}



BiomeFilter setupBiomeFilter(const int *biomeList, int listLen)
{
    BiomeFilter bf;
    int i, id;

    memset(&bf, 0, sizeof(bf));

    for (i = 0; i < listLen; i++)
    {
        id = biomeList[i] & 0x7f;
        switch (id)
        {
        case mushroomIsland:
        case mushroomIslandShore:
            bf.requireMushroom = 1;
            bf.tempCat |= (1ULL << Oceanic);
            bf.biomesToFind |= (1ULL << id);
        case mesa:
        case mesaPlateau_F:
        case mesaPlateau:
            bf.tempCat |= (1ULL << (Warm+Special));
            bf.biomesToFind |= (1ULL << id);
            break;
        case savanna:
        case savannaPlateau:
            bf.tempCat |= (1ULL << Warm);
            bf.biomesToFind |= (1ULL << id);
            break;
        case roofedForest:
        case birchForest:
        case birchForestHills:
        case swampland:
            bf.tempCat |= (1ULL << Lush);
            bf.biomesToFind |= (1ULL << id);
            break;
        case jungle:
        case jungleHills:
            bf.tempCat |= (1ULL << (Lush+Special));
            bf.biomesToFind |= (1ULL << id);
            break;
        /*case jungleEdge:
            bf.tempCat |= (1ULL << Lush) | (1ULL << Lush+4);
            bf.biomesToFind |= (1ULL << id);
            break;*/
        case megaTaiga:
        case megaTaigaHills:
            bf.tempCat |= (1ULL << (Cold+Special));
            bf.biomesToFind |= (1ULL << id);
            break;
        case icePlains:
        case iceMountains:
        case coldTaiga:
        case coldTaigaHills:
            bf.tempCat |= (1ULL << Freezing);
            bf.biomesToFind |= (1ULL << id);
            break;
        default:
            bf.biomesToFind |= (1ULL << id);

            if (isOceanic(id))
            {
                if (id != ocean && id != deepOcean)
                    bf.doOceanTypeCheck = 1;

                if (isShallowOcean(id))
                {
                    bf.oceansToFind |= (1ULL < id);
                }
                else
                {
                    if (id == warmDeepOcean)
                        bf.oceansToFind |= (1ULL << warmOcean);
                    else if (id == lukewarmDeepOcean)
                        bf.oceansToFind |= (1ULL << lukewarmOcean);
                    else if (id == deepOcean)
                        bf.oceansToFind |= (1ULL << ocean);
                    else if (id == coldDeepOcean)
                        bf.oceansToFind |= (1ULL << coldOcean);
                    else if (id == frozenDeepOcean)
                        bf.oceansToFind |= (1ULL << frozenOcean);
                }
                bf.tempCat |= (1ULL << Oceanic);
            }
            else
            {
                bf.biomesToFind |= (1ULL << id);
            }
            break;
        }
    }

    for (i = 0; i < Special; i++)
    {
        if (bf.tempCat & (1ULL << i)) bf.tempNormal++;
    }
    for (i = Special; i < Freezing+Special; i++)
    {
        if (bf.tempCat & (1ULL << i)) bf.tempSpecial++;
    }

    bf.doTempCheck = (bf.tempSpecial + bf.tempNormal) >= 6;
    bf.doShroomAndTempCheck = bf.requireMushroom && (bf.tempSpecial >= 1 || bf.tempNormal >= 4);
    bf.doMajorBiomeCheck = 1;
    bf.checkBiomePotential = 1;
    bf.doScale4Check = 1;

    return bf;
}



/* Tries to determine if the biomes configured in the filter will generate in
 * this seed within the specified area. The smallest layer scale checked is
 * given by 'minscale'. Lowering this value terminate the search earlier and
 * yield more false positives.
 */
int64_t checkForBiomes(
        LayerStack *        g,
        int *               cache,
        const int64_t       seed,
        const int           blockX,
        const int           blockZ,
        const unsigned int  width,
        const unsigned int  height,
        const BiomeFilter   filter,
        const int           minscale)
{
    Layer *lspecial = &g->layers[L_SPECIAL_1024];
    Layer *lmushroom = &g->layers[L_ADD_MUSHROOM_ISLAND_256];
    Layer *lbiomes = &g->layers[L_BIOME_256];
    Layer *loceantemp = NULL;

    int *map = cache ? cache : allocCache(&g->layers[g->layerNum-1], width, height);

    uint64_t potential, required, modified;
    int64_t ss, cs;
    int id, types[0x100];
    int i, x, z;
    int areaX1024, areaZ1024, areaWidth1024, areaHeight1024;
    int areaX256, areaZ256, areaWidth256, areaHeight256;
    int areaX4, areaZ4, areaWidth4, areaHeight4;

    // 1:1024 scale
    areaX1024 = blockX >> 10;
    areaZ1024 = blockZ >> 10;
    areaWidth1024 = ((width-1) >> 10) + 2;
    areaHeight1024 = ((height-1) >> 10) + 2;

    // 1:256 scale
    areaX256 = blockX >> 8;
    areaZ256 = blockZ >> 8;
    areaWidth256 = ((width-1) >> 8) + 2;
    areaHeight256 = ((height-1) >> 8) + 2;


    /*** BIOME CHECKS THAT DON'T NEED OTHER LAYERS ***/

    // Check that there is the necessary minimum of both special and normal
    // temperature categories present.
    if (filter.tempNormal || filter.tempSpecial)
    {
        ss = processWorldSeed(seed, lspecial->baseSeed);

        types[0] = types[1] = 0;
        for (z = 0; z < areaHeight1024; z++)
        {
            for (x = 0; x < areaWidth1024; x++)
            {
                cs = getChunkSeed(ss, (int64_t)(x + areaX1024), (int64_t)(z + areaZ1024));
                types[(cs >> 24) % 13 == 0]++;
            }
        }

        if (types[0] < filter.tempNormal || types[1] < filter.tempSpecial)
        {
            goto return_zero;
        }
    }

    // Check there is a mushroom island, provided there is an ocean.
    if (filter.requireMushroom)
    {
        ss = processWorldSeed(seed, lmushroom->baseSeed);

        for (z = 0; z < areaHeight256; z++)
        {
            for (x = 0; x < areaWidth256; x++)
            {
                cs = getChunkSeed(ss, (int64_t)(x + areaX256), (int64_t)(z + areaZ256));
                if ((cs >> 24) % 100 == 0)
                {
                    goto after_protomushroom;
                }
            }
        }

        goto return_zero;
    }
    after_protomushroom:

    if (filter.checkBiomePotential)
    {
        ss = processWorldSeed(seed, lbiomes->baseSeed);

        potential = 0;
        required = filter.biomesToFind & (
                (1ULL << mesaPlateau) | (1ULL << mesaPlateau_F) |
                (1ULL << savanna)     | (1ULL << roofedForest) |
                (1ULL << birchForest) | (1ULL << swampland) );

        for (z = 0; z < areaHeight256; z++)
        {
            for (x = 0; x < areaWidth256; x++)
            {
                cs = getChunkSeed(ss, (int64_t)(x + areaX256), (int64_t)(z + areaZ256));
                cs >>= 24;

                int cs6 = cs % 6;
                int cs3 = cs6 & 3;

                if (cs3 == 0) potential |= (1ULL << mesaPlateau);
                else if (cs3 == 1 || cs3 == 2) potential |= (1ULL << mesaPlateau_F);

                if (cs6 == 1) potential |= (1ULL << roofedForest);
                else if (cs6 == 3) potential |= (1ULL << savanna);
                else if (cs6 == 4) potential |= (1ULL << savanna) | (1ULL << birchForest);
                else if (cs6 == 5) potential |= (1ULL << swampland);

                if (!((potential & required) ^ required))
                {
                    goto after_protobiome;
                }
            }
        }

        goto return_zero;
    }
    after_protobiome:


    /*** BIOME CHECKS ***/

    if (filter.doTempCheck)
    {
        setWorldSeed(lspecial, seed);
        genArea(lspecial, map, areaX1024, areaZ1024, areaWidth1024, areaHeight1024);

        potential = 0;

        for (i = 0; i < areaWidth1024 * areaHeight1024; i++)
        {
            id = map[i];
            if (id >= Special) id = (id & 0xf) + Special;
            potential |= (1ULL << id);
        }

        if ((potential & filter.tempCat) ^ filter.tempCat)
        {
            goto return_zero;
        }
    }

    if (minscale > 256) goto return_one;

    if (filter.doShroomAndTempCheck)
    {
        setWorldSeed(lmushroom, seed);
        genArea(lmushroom, map, areaX256, areaZ256, areaWidth256, areaHeight256);

        potential = 0;

        for (i = 0; i < areaWidth256 * areaHeight256; i++)
        {
            id = map[i];
            if (id >= BIOME_NUM) id = (id & 0xf) + Special;
            potential |= (1ULL << id);
        }

        required = filter.tempCat | (1ULL << mushroomIsland);

        if ((potential & required) ^ required)
        {
            goto return_zero;
        }
    }

    if (filter.doOceanTypeCheck)
    {
        loceantemp = &g->layers[L13_OCEAN_TEMP_256];
        setWorldSeed(loceantemp, seed);
        genArea(loceantemp, map, areaX256, areaZ256, areaWidth256, areaHeight256);

        potential = 0; // ocean potential

        for (i = 0; i < areaWidth256 * areaHeight256; i++)
        {
            id = map[i];
            if (id == warmOcean)     potential |= (1ULL << warmOcean) | (1ULL << lukewarmOcean);
            if (id == lukewarmOcean) potential |= (1ULL << lukewarmOcean);
            if (id == ocean)         potential |= (1ULL << ocean);
            if (id == coldOcean)     potential |= (1ULL << coldOcean);
            if (id == frozenOcean)   potential |= (1ULL << frozenOcean) | (1ULL << coldOcean);
        }

        if ((potential & filter.oceansToFind) ^ filter.oceansToFind)
        {
            goto return_zero;
        }
    }

    if (filter.doMajorBiomeCheck)
    {
        setWorldSeed(lbiomes, seed);
        genArea(lbiomes, map, areaX256, areaZ256, areaWidth256, areaHeight256);

        // get biomes out of the way that we cannot check for at this layer
        potential = (1ULL << beach) | (1ULL << stoneBeach) | (1ULL << coldBeach) |
                (1ULL << river) | (1ULL << frozenRiver);

        for (i = 0; i < areaWidth256 * areaHeight256; i++)
        {
            id = map[i];
            switch (id)
            {
            case mesaPlateau_F:
            case mesaPlateau:
                potential |= (1ULL << id) | (1ULL << mesa) | (1ULL << desert); break;
            case megaTaiga:
                potential |= (1ULL << id) | (1ULL << taiga) | (1ULL << taigaHills) | (1ULL << megaTaigaHills); break;
            case desert:
                potential |= (1ULL << id) | (1ULL << extremeHillsPlus) | (1ULL << desertHills); break;
            case swampland:
                potential |= (1ULL << id) | (1ULL << jungleEdge) | (1ULL << plains); break;
            case forest:
                potential |= (1ULL << id) | (1ULL << forestHills); break;
            case birchForest:
                potential |= (1ULL << id) | (1ULL << birchForestHills); break;
            case roofedForest:
                potential |= (1ULL << id) | (1ULL << plains); break;
            case taiga:
                potential |= (1ULL << id) | (1ULL << taigaHills); break;
            case coldTaiga:
                potential |= (1ULL << id) | (1ULL << coldTaigaHills); break;
            case plains:
                potential |= (1ULL << id) | (1ULL << forestHills) | (1ULL << forest); break;
            case icePlains:
                potential |= (1ULL << id) | (1ULL << iceMountains); break;
            case jungle:
                potential |= (1ULL << id) | (1ULL << jungleHills); break;
            case ocean:
                potential |= (1ULL << id) | (1ULL << deepOcean);
                // TODO: buffer possible ocean types at this location
                potential |= (1ULL << frozenOcean) | (1ULL << coldOcean) | (1ULL << lukewarmOcean) | (1ULL << warmOcean); break;
            case extremeHills:
                potential |= (1ULL << id) | (1ULL << extremeHillsPlus); break;
            case savanna:
                potential |= (1ULL << id) | (1ULL << savannaPlateau); break;
            case deepOcean:
                potential |= (1ULL << id) | (1ULL << plains) | (1ULL << forest);
                // TODO: buffer possible ocean types at this location
                potential |= (1ULL << frozenDeepOcean) | (1ULL << coldDeepOcean) | (1ULL << lukewarmDeepOcean); break;
            case mushroomIsland:
                potential |= (1ULL << id) | (1ULL << mushroomIslandShore);
            default:
                potential |= (1ULL << id);
            }
        }

        if ((potential & filter.biomesToFind) ^ filter.biomesToFind)
        {
            goto return_zero;
        }
    }

    // TODO: Do a check at the HILLS layer, scale 1:64

    if (minscale > 4) goto return_one;

    if (filter.doScale4Check)
    {
        // 1:4 scale
        areaX4 = blockX >> 2;
        areaZ4 = blockZ >> 2;
        areaWidth4 = ((width-1) >> 2) + 2;
        areaHeight4 = ((height-1) >> 2) + 2;

        applySeed(g, seed);
        genArea(&g->layers[g->layerNum-2], map, areaX4, areaZ4, areaWidth4, areaHeight4);

        potential = modified = 0;

        for (i = 0; i < areaWidth4 * areaHeight4; i++)
        {
            id = map[i];
            if (id >= 128) modified |= (1ULL << (id & 0x7f));
            else potential |= (1ULL << id);
        }

        required = filter.biomesToFind;
        if ((potential & required) ^ required)
        {
            goto return_zero;
        }
        if ((modified & filter.modifiedToFind) ^ filter.modifiedToFind)
        {
            goto return_zero;
        }
    }


    int ret;

    // clean up and return
    return_one:
    ret = 1;

    if (0)
    {
        return_zero:
        ret = 0;
    }

    if (cache == NULL) free(map);

    return ret;
}












