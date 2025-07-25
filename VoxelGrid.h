#pragma once
#include "dll_global.h"

#include <vector>
#include "Vertex.h"

struct DPVISION_DLL_API VoxelGrid {
    std::vector<uint8_t> data;
    float voxelSize;
    int dimX, dimY, dimZ;
    float minX, minY, minZ;

    uint8_t& at(int x, int y, int z) {
        return data[x + y * dimX + z * dimX * dimY];
    }

    const uint8_t& at(int x, int y, int z) const {
        return data[x + y * dimX + z * dimX * dimY];
    }

    bool inBounds(int x, int y, int z) const {
        return x >= 0 && y >= 0 && z >= 0 &&
            x < dimX && y < dimY && z < dimZ;
    }

    CVertex voxelCenter(int x, int y, int z) const;

    void allocateLike(const VoxelGrid& other);

    void negate(const VoxelGrid& input);
    void sum_in_place(const VoxelGrid& input);
    void diff_in_place(const VoxelGrid& input);
    void diff_of(const VoxelGrid& input1, const VoxelGrid& input2);
    void intersection_of(const VoxelGrid& input1, const VoxelGrid& input2);

    void fillDown();
    void fillUp();

    static void sum(const VoxelGrid & input, const VoxelGrid & input2, VoxelGrid & output);
};
