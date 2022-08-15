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

#include <iostream>
#include <algorithm>
#include "dxf_iface.h"
#include "libdxfrw/libdwgr.h"
#include "libdxfrw/libdxfrw.h"

#include "parser/contexts/transformable-context.h"

dxf_iface::dxf_iface() {
}

bool dxf_iface::printText(Document *doc, const std::string& fileI, dxf_data *fData, QList<LayerPtr> *svg_layers) {
    unsigned int found = fileI.find_last_of(".");
    std::string fileExt = fileI.substr(found+1);
    std::transform(fileExt.begin(), fileExt.end(),fileExt.begin(), ::toupper);

    bool success = false;
    if (fileExt == "DXF"){
        //loads dxf
        dxfRW* dxf = new dxfRW(fileI.c_str());
        success = dxf->read(this, false);
        delete dxf;
    } else if (fileExt == "DWG"){
        //loads dwg
        dwgR* dwg = new dwgR(fileI.c_str());
        success = dwg->read(this, false);
        delete dwg;
    }

    if (success) {
        qInfo() << "Total layers" << dxf_layers_.size();
        for (auto &layer : dxf_layers_) {
            doc->addLayer(layer);
            svg_layers->push_back(layer);
        }
    }
    return success;
}

void dxf_iface::addLayer(const DRW_Layer& data) {
    if(data.name.compare("0") != 0) {
        layer_ptr_ = std::make_shared<Layer>();
        layer_ptr_->setName(QString::fromStdString(data.name));
        dxf_layers_.push_back(layer_ptr_);
    }

    std::cout << __func__ << " " << data.name << " " << data.color << std::endl;
}

void dxf_iface::addPolyline(const DRW_Polyline& data) {
    if(layer_ptr_ == nullptr) {
        return;
    }
    QPainterPath working_path;
    working_path.moveTo(data.vertlist[0]->basePoint.x, data.vertlist[0]->basePoint.y);
    for(unsigned int i = 1; i < data.vertlist.size(); ++i) {
        std::cout << data.vertlist[i]->basePoint.x << " " << data.vertlist[i]->basePoint.y << std::endl;
        working_path.lineTo(data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y);
    }
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    layer_ptr_->addShape(new_shape);
}

void dxf_iface::addSpline(const DRW_Spline* data) {
    if(layer_ptr_ == nullptr) {
        return;
    }
    QPainterPath working_path;
    working_path.moveTo(data->controllist[0]->x, data->controllist[0]->y);
    for(unsigned int i = 1; (i+2) < data->controllist.size(); i+=3) {
        working_path.cubicTo(data->controllist[i]->x, data->controllist[i]->y, 
                              data->controllist[i+1]->x, data->controllist[i+1]->y,
                              data->controllist[i+2]->x, data->controllist[i+2]->y);
    }
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    layer_ptr_->addShape(new_shape);

    bool closed = data->flags & (1 << 0);
    std::cout << "closed = " << closed << std::endl;
    std::cout << "controllist(" << data->ncontrol << "):" << std::endl;
    int degree = data->degree;
    std::cout << "degree = " << degree << std::endl;
    for(unsigned int i = 0; i < data->controllist.size(); ++i)
    {
        std::cout << data->controllist[i]->x << " " << data->controllist[i]->y << std::endl;
    }
}