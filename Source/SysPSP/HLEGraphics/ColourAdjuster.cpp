#include "Base/Types.h"
#include <cstdint>
#include <vector>
#include "HLEGraphics/DaedalusVtx.h"
#include "SysPSP/HLEGraphics/ColourAdjuster.h"

//*****************************************************************************
//
//*****************************************************************************
void CColourAdjuster::Reset()
{
    mModulateMask = 0;
    mSetMask = 0;
    mSubtractMask = 0;
}

//*****************************************************************************
//
//*****************************************************************************
void CColourAdjuster::Process(DaedalusVtx* p_vertices, std::size_t num_verts) const
{
#ifdef DAEDALUS_ENABLE_ASSERTS
    DAEDALUS_ASSERT((mSetMask & mModulateMask & mSubtractMask) == 0, "Setting and modulating the same component");
#endif

    if (mSetMask) {
        u32 clear_bits = ~mSetMask;
        u32 set_bits = mSetColour.GetColour() & mSetMask;

        for (std::size_t v = 0; v < num_verts; ++v) {
            p_vertices[v].Colour = c32((p_vertices[v].Colour.GetColour() & clear_bits) | set_bits);
        }
    }

    if (mSubtractMask) {
        switch (mSubtractMask) {
        case COL32_MASK_RGB:
            for (std::size_t v = 0; v < num_verts; ++v) {
                p_vertices[v].Colour = p_vertices[v].Colour.SubRGB(mSubtractColour);
            }
            break;

        case COL32_MASK_A:
            for (std::size_t v = 0; v < num_verts; ++v) {
                p_vertices[v].Colour = p_vertices[v].Colour.SubA(mSubtractColour);
            }
            break;

        case COL32_MASK_RGBA:
            for (std::size_t v = 0; v < num_verts; ++v) {
                p_vertices[v].Colour = p_vertices[v].Colour.Sub(mSubtractColour);
            }
            break;
        }
    }

    if (mModulateMask) {
        switch (mModulateMask) {
        case COL32_MASK_RGB:
            for (std::size_t v = 0; v < num_verts; ++v) {
                p_vertices[v].Colour = p_vertices[v].Colour.ModulateRGB(mModulateColour);
            }
            break;

        case COL32_MASK_A:
            for (std::size_t v = 0; v < num_verts; ++v) {
                p_vertices[v].Colour = p_vertices[v].Colour.ModulateA(mModulateColour);
            }
            break;

        case COL32_MASK_RGBA:
            for (std::size_t v = 0; v < num_verts; ++v) {
                p_vertices[v].Colour = p_vertices[v].Colour.Modulate(mModulateColour);
            }
            break;
        }
    }
}

//*****************************************************************************
//
//*****************************************************************************
void CColourAdjuster::Set(u32 mask, c32 colour)
{
#ifdef DAEDALUS_ENABLE_ASSERTS
    DAEDALUS_ASSERT((GetMask() & mask) == 0, "These bits have already been set");
#endif

    mSetMask |= mask;

    // Using bitwise operations to update the color
    mSetColour = c32((mSetColour.GetColour() & ~mask) | (colour.GetColour() & mask));
}

//*****************************************************************************
//
//*****************************************************************************
void CColourAdjuster::Modulate(u32 mask, c32 colour)
{
#ifdef DAEDALUS_ENABLE_ASSERTS
    DAEDALUS_ASSERT((GetMask() & mask) == 0, "These bits have already been set");
#endif

    mModulateMask |= mask;

    // Using bitwise operations to update the modulate color
    mModulateColour = c32((mModulateColour.GetColour() & ~mask) | (colour.GetColour() & mask));
}

//*****************************************************************************
//
//*****************************************************************************
void CColourAdjuster::Subtract(u32 mask, c32 colour)
{
#ifdef DAEDALUS_ENABLE_ASSERTS
    DAEDALUS_ASSERT((GetMask() & mask) == 0, "These bits have already been set");
#endif

    mSubtractMask |= mask;

    // Using bitwise operations to update the subtract color
    mSubtractColour = c32((mSubtractColour.GetColour() & ~mask) | (colour.GetColour() & mask));
}
