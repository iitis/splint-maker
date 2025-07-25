#include "VoxelGrid.h"

#include <omp.h>

CVertex VoxelGrid::voxelCenter(int x, int y, int z) const {
    return CVertex(
        minX + (x + 0.5f) * voxelSize,
        minY + (y + 0.5f) * voxelSize,
        minZ + (z + 0.5f) * voxelSize
    );
}

void VoxelGrid::allocateLike(const VoxelGrid& other) {
    minX = other.minX;
    minY = other.minY;
    minZ = other.minZ;
    dimX = other.dimX;
    dimY = other.dimY;
    dimZ = other.dimZ;
    voxelSize = other.voxelSize;
    data.resize((size_t)dimX * dimY * dimZ, 0);
}

void VoxelGrid::negate(const VoxelGrid& input)
{
#pragma omp parallel for
    for (int i = 0; i < (int)data.size(); ++i)
        data[i] = (input.data[i]) ? 0 : 1;
}


void VoxelGrid::sum_in_place(const VoxelGrid& input)
{
#pragma omp parallel for
    for (int i = 0; i < (int)data.size(); ++i)
        data[i] = (data[i] || input.data[i]) ? 1 : 0;
}

void VoxelGrid::diff_in_place(const VoxelGrid& input)
{
#pragma omp parallel for
    for (int i = 0; i < (int)data.size(); ++i)
        data[i] = (data[i] && !input.data[i]) ? 1 : 0;
}

void VoxelGrid::diff_of(const VoxelGrid& input1, const VoxelGrid& input2)
{
#pragma omp parallel for
    for (int i = 0; i < (int)data.size(); ++i)
        data[i] = (input1.data[i] && !input2.data[i]) ? 1 : 0;
}

void VoxelGrid::intersection_of(const VoxelGrid& input1, const VoxelGrid& input2)
{
#pragma omp parallel for
    for (int i = 0; i < (int)data.size(); ++i)
        data[i] = (input1.data[i] && input2.data[i]) ? 1 : 0;
}

void VoxelGrid::sum(const VoxelGrid& input, const VoxelGrid & input2, VoxelGrid& output)
{
    output.allocateLike(input);

#pragma omp parallel for
    for (int i = 0; i < (int)input.data.size(); ++i)
        output.data[i] = (input.data[i] || input2.data[i]) ? 0 : 1;
}

void VoxelGrid::fillDown() {
#pragma omp parallel for
    for (int r = 0; r < dimY; r++)
        for (int c = 0; c < dimX; c++)
        {
            int l = dimZ - 1;

            while ((l >= 0) && (this->at(c, r, l) == 0))
                l--;

            while (l >= 0)
            {
                this->at(c, r, l) = 1;
                l--;
            }
        }
}

void VoxelGrid::fillUp() {
#pragma omp parallel for
    for (int r = 0; r < dimY; r++)
        for (int c = 0; c < dimX; c++)
        {
            int l = 0;

            while ((l < dimZ) && (this->at(c, r, l) == 0))
                l++;

            while (l < dimZ) {
                this->at(c, r, l) = 1;
                l++;
            }
        }
}
