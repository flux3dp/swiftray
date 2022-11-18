/******************************************************************************
**  dwg2text - Program to convert dwg/dxf to dxf(ascii & binary)             **
**                                                                           **
**  Copyright (C) 2015 José F. Soriano, rallazz@gmail.com                    **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DX_IFACE_H
#define DX_IFACE_H

#include "libdxfrw/drw_interface.h"
#include "libdxfrw/libdxfrw.h"
#include "dxf_data.h"
#include <iostream>

#include <QPainterPath>
#include "layer.h"
#include "document.h"

class dxf_iface : public DRW_Interface {
public:
    dxf_iface();
    ~dxf_iface(){}
    bool printText(Document *doc, const std::string& fileI, dxf_data *fData, QList<LayerPtr> *svg_layers);

//reimplement virtual DRW_Interface functions

//reader part, stores all in class dxf_data
    //header
    void addHeader(const DRW_Header* data) {}

    //tables
    virtual void addLType(const DRW_LType& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addLayer(const DRW_Layer& data);
    virtual void addDimStyle(const DRW_Dimstyle& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addVport(const DRW_Vport& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addTextStyle(const DRW_Textstyle& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addAppId(const DRW_AppId& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }

    //blocks
    virtual void addBlock(const DRW_Block& data);
    virtual void endBlock(){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }

    virtual void setBlock(const int /*handle*/){}//unused

    //entities
    virtual void addPoint(const DRW_Point& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addLine(const DRW_Line& data);
    virtual void addRay(const DRW_Ray& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addXline(const DRW_Xline& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addArc(const DRW_Arc& data);
    virtual void addCircle(const DRW_Circle& data);
    virtual void addEllipse(const DRW_Ellipse& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addLWPolyline(const DRW_LWPolyline& data);
    virtual void addPolyline(const DRW_Polyline& data);
    virtual void addSpline(const DRW_Spline* data);
    // ¿para que se usa?
    virtual void addKnot(const DRW_Entity& data){}

    virtual void addInsert(const DRW_Insert& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addTrace(const DRW_Trace& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void add3dFace(const DRW_3Dface& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addSolid(const DRW_Solid& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addMText(const DRW_MText& data){
        // std::cout << __func__ << " " << data.text << " " << data.height << std::endl;
    }
    virtual void addText(const DRW_Text& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addDimAlign(const DRW_DimAligned *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addDimLinear(const DRW_DimLinear *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addDimRadial(const DRW_DimRadial *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addDimDiametric(const DRW_DimDiametric *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addDimAngular(const DRW_DimAngular *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addDimAngular3P(const DRW_DimAngular3p *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addDimOrdinate(const DRW_DimOrdinate *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addLeader(const DRW_Leader *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addHatch(const DRW_Hatch *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addViewport(const DRW_Viewport& data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }
    virtual void addImage(const DRW_Image *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }

    virtual void linkImage(const DRW_ImageDef *data){
        // std::cout << __func__ << " " << __LINE__ << std::endl;
    }

//writer part, send all in class dxf_data to writer
    void addComment(const char* comment){}
    void addPlotSettings(const DRW_PlotSettings *data){}

    void writeHeader(DRW_Header& data) {};
	void writeBlocks() {};
	void writeBlockRecords() {};
	void writeEntities() {};
	void writeLTypes() {};
	void writeLayers() {};
	void writeTextstyles() {};
	void writeVports() {};
	void writeDimstyles() {};
    void writeObjects() {};
	void writeAppId() {};

private:
    LayerPtr layer_ptr_ = nullptr;
    QList<LayerPtr> dxf_layers_;
    QString current_block_;
    QMap<QString, QList<ShapePtr> > g_block2shape_map_;
};

#endif // DX_IFACE_H
