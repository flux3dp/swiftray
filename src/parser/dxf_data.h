/******************************************************************************
**  dwg2dxf - Program to convert dwg/dxf to dxf(ascii & binary)              **
**                                                                           **
**  Copyright (C) 2015 José F. Soriano, rallazz@gmail.com                    **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DX_DATA_H
#define DX_DATA_H
#include "libdxfrw/libdxfrw.h"

//class to store image data and path from DRW_ImageDef
class dxf_ifaceImg : public DRW_Image {
public:
    dxf_ifaceImg(){}
    dxf_ifaceImg(const DRW_Image& p):DRW_Image(p){}
    ~dxf_ifaceImg(){}
    std::string path; //stores the image path
};

//container class to store entites.
class dxf_ifaceBlock : public DRW_Block {
public:
    dxf_ifaceBlock(){}
    dxf_ifaceBlock(const DRW_Block& p):DRW_Block(p){}
    ~dxf_ifaceBlock(){
        for (std::list<DRW_Entity*>::const_iterator it=ent.begin(); it!=ent.end(); ++it)
            delete *it;
    }
    std::list<DRW_Entity*>ent; //stores the entities list
};


//container class to store full dwg/dxf data.
class dxf_data {
public:
    dxf_data(){
        mBlock = new dxf_ifaceBlock();
    }
    ~dxf_data(){
        //cleanup,
        for (std::list<dxf_ifaceBlock*>::const_iterator it=blocks.begin(); it!=blocks.end(); ++it)
            delete *it;
        delete mBlock;
    }

    DRW_Header headerC;                 //stores a copy of the header vars
    std::list<DRW_LType>lineTypes;      //stores a copy of all line types
    std::list<DRW_Layer>layers;         //stores a copy of all layers
    std::list<DRW_Dimstyle>dimStyles;   //stores a copy of all dimension styles
    std::list<DRW_Vport>VPorts;         //stores a copy of all vports
    std::list<DRW_Textstyle>textStyles; //stores a copy of all text styles
    std::list<DRW_AppId>appIds;         //stores a copy of all line types
    std::list<dxf_ifaceBlock*>blocks;    //stores a copy of all blocks and the entities in it
    std::list<dxf_ifaceImg*>images;      //temporary list to find images for link with DRW_ImageDef. Do not delete it!!

    dxf_ifaceBlock* mBlock;              //container to store model entities


};

#endif // DX_DATA_H
