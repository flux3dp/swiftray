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

bool dxf_iface::printText(Document *doc, QString fileI, dxf_data *fData, QList<LayerPtr> *svg_layers) {
    bool success = false;
    if (fileI.toLower().endsWith(".dxf")){
        //loads dxf
        dxfRW* dxf = new dxfRW(fileI.toStdString().c_str());
        success = dxf->read(this, false);
        delete dxf;
    } else if (fileI.toLower().endsWith(".dwg")){
        //loads dwg
        dwgR* dwg = new dwgR(fileI.toStdString().c_str());
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
    layer_ptr_ = std::make_shared<Layer>();
    layer_ptr_->setName(QString::fromStdString(data.name));
    dxf_layers_.push_back(layer_ptr_);

    // std::cout << __func__ << " " << data.name << " " << data.color << std::endl;
}

void dxf_iface::addLine(const DRW_Line& data) {
    // std::cout << __func__ << " " << __LINE__ << std::endl;
    if(layer_ptr_ == nullptr) {
        return;
    }
    // std::cout << __func__ << " basePoint.x = " << data.basePoint.x << " basePoint.y = " << data.basePoint.y << std::endl;
    // std::cout << __func__ << " secPoint.x = " << data.secPoint.x << " secPoint.y = " << data.secPoint.y << std::endl;
    QPainterPath working_path;
    working_path.moveTo(data.basePoint.x, data.basePoint.y);
    working_path.lineTo(data.secPoint.x, data.secPoint.y);
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    layer_ptr_->addShape(new_shape);
}

void dxf_iface::addArc(const DRW_Arc& data) {
    if(layer_ptr_ == nullptr) {
        return;
    }
    QPainterPath working_path;
    working_path.moveTo(data.basePoint.x + data.radious * cos(data.staangle), data.basePoint.y + data.radious * sin(data.staangle));
    for(int i = 0 ; (i+2)<= 71; i+= 3) {
        if(data.staangle < data.endangle) {
            working_path.cubicTo(data.basePoint.x + data.radious * cos((double)i/71 * (data.endangle-data.staangle) + data.staangle),  
                                data.basePoint.y + data.radious * sin((double)i/71 * (data.endangle-data.staangle) + data.staangle), 
                                data.basePoint.x + data.radious * cos((double)(i+1)/71 * (data.endangle-data.staangle) + data.staangle),  
                                data.basePoint.y + data.radious * sin((double)(i+1)/71 * (data.endangle-data.staangle) + data.staangle), 
                                data.basePoint.x + data.radious * cos((double)(i+2)/71 * (data.endangle-data.staangle) + data.staangle),  
                                data.basePoint.y + data.radious * sin((double)(i+2)/71 * (data.endangle-data.staangle) + data.staangle));
        } else {
            working_path.cubicTo(data.basePoint.x + data.radious * cos((double)i/71 * (2*M_PI+data.endangle-data.staangle) + data.staangle), 
                                data.basePoint.y + data.radious * sin((double)i/71 * (2*M_PI+data.endangle-data.staangle) + data.staangle), 
                                data.basePoint.x + data.radious * cos((double)(i+1)/71 * (2*M_PI+data.endangle-data.staangle) + data.staangle), 
                                data.basePoint.y + data.radious * sin((double)(i+1)/71 * (2*M_PI+data.endangle-data.staangle) + data.staangle), 
                                data.basePoint.x + data.radious * cos((double)(i+2)/71 * (2*M_PI+data.endangle-data.staangle) + data.staangle), 
                                data.basePoint.y + data.radious * sin((double)(i+2)/71 * (2*M_PI+data.endangle-data.staangle) + data.staangle));
        }
    }
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    layer_ptr_->addShape(new_shape);
}

void dxf_iface::addCircle(const DRW_Circle& data) {
    if(layer_ptr_ == nullptr) {
        return;
    }
    QPointF center(data.basePoint.x, data.basePoint.y);
    QPainterPath working_path;
    working_path.addEllipse(center, data.radious, data.radious);
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    layer_ptr_->addShape(new_shape);
}

void dxf_iface::addLWPolyline(const DRW_LWPolyline& data) {
    // std::cout << __func__ << " " << __LINE__ << std::endl;
    if(layer_ptr_ == nullptr) {
        return;
    }
    QPainterPath working_path;
    working_path.moveTo(data.vertlist[0]->x, data.vertlist[0]->y);
    for(unsigned int i = 1; i < data.vertlist.size(); ++i) {
        working_path.lineTo(data.vertlist[i]->x, data.vertlist[i]->y);
    }
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    layer_ptr_->addShape(new_shape);
}

void dxf_iface::addPolyline(const DRW_Polyline& data) {
    if(layer_ptr_ == nullptr) {
        return;
    }
    QPainterPath working_path;
    working_path.moveTo(data.vertlist[0]->basePoint.x, data.vertlist[0]->basePoint.y);
    for(unsigned int i = 1; i < data.vertlist.size(); ++i) {
        // std::cout << data.vertlist[i]->basePoint.x << " " << data.vertlist[i]->basePoint.y << std::endl;
        working_path.lineTo(data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y);
    }
    // bool closed = data.flags & (1 << 0);
    // std::cout << "closed = " << closed << std::endl;
    // if(closed) {
    //     working_path.lineTo(data.vertlist[0]->basePoint.x, data.vertlist[0]->basePoint.y);
    // }
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    layer_ptr_->addShape(new_shape);
}

void dxf_iface::addSpline(const DRW_Spline* data) {
    if(layer_ptr_ == nullptr) {
        return;
    }
    QPainterPath working_path;
    working_path.moveTo(data->controllist[0]->x, data->controllist[0]->y);
    // std::cout << __func__ << " controllist.size() = " << data->controllist.size() << std::endl;
    unsigned int index = 1;
    for(; (index+2) < data->controllist.size(); index+=3) {
        working_path.cubicTo(data->controllist[index]->x, data->controllist[index]->y, 
                              data->controllist[index+1]->x, data->controllist[index+1]->y,
                              data->controllist[index+2]->x, data->controllist[index+2]->y);
    }
    // std::cout << __func__ << " index = " << index << std::endl;
    if(index != data->controllist.size()) {
        working_path.cubicTo(data->controllist[data->controllist.size()-3]->x, data->controllist[data->controllist.size()-3]->y, 
                              data->controllist[data->controllist.size()-2]->x, data->controllist[data->controllist.size()-2]->y,
                              data->controllist[data->controllist.size()-1]->x, data->controllist[data->controllist.size()-1]->y);
    }

    // bool closed = data->flags & (1 << 0);
    // std::cout << "closed = " << closed << std::endl;
    // if(closed) {
    //     working_path.cubicTo(data->controllist[data->controllist.size()-2]->x, data->controllist[data->controllist.size()-2]->y, 
    //                           data->controllist[data->controllist.size()-1]->x, data->controllist[data->controllist.size()-1]->y,
    //                           data->controllist[0]->x, data->controllist[0]->y);
    // }
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    layer_ptr_->addShape(new_shape);
    // std::cout << "controllist(" << data->ncontrol << "):" << std::endl;
    // int degree = data->degree;
    // std::cout << "degree = " << degree << std::endl;
    // for(unsigned int i = 0; i < data->controllist.size(); ++i)
    // {
    //     std::cout << data->controllist[i]->x << " " << data->controllist[i]->y << std::endl;
    // }
}