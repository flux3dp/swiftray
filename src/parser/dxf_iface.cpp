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
#include <QtMath>
#include "rs_handle/rs_vector.h"
#include "dxf_iface.h"
#include "libdxfrw/libdwgr.h"
#include "libdxfrw/libdxfrw.h"

#include "parser/contexts/transformable-context.h"

dxf_iface::dxf_iface() {
}

/**
 * Converts a DXF encoded string into a native Unicode string.
 */
QString toNativeString(const QString& data) {
    QString res;

    // Ignore font tags:
    int j = 0;
    for (int i=0; i<data.length(); ++i) {
        if (data.at(i).unicode() == 0x7B){ //is '{' ?
            if (data.at(i+1).unicode() == 0x5c){ //and is "{\" ?
                //check known codes
                if ( (data.at(i+2).unicode() == 0x66) || //is "\f" ?
                     (data.at(i+2).unicode() == 0x48) || //is "\H" ?
                     (data.at(i+2).unicode() == 0x43)    //is "\C" ?
                   ) {
                    //found tag, append parsed part
                    res.append(data.mid(j,i-j));
                    int pos = data.indexOf(0x7D, i+3);//find '}'
                    if (pos <0) break; //'}' not found
                    QString tmp = data.mid(i+1, pos-i-1);
                    do {
                        tmp = tmp.remove(0,tmp.indexOf(0x3B, 0)+1 );//remove to ';'
                    } while(tmp.startsWith("\\f") || tmp.startsWith("\\H") || tmp.startsWith("\\C"));
                    res.append(tmp);
                    i = j = pos;
                    ++j;
                }
            }
        }
    }
    res.append(data.mid(j));

    // Line feed:
    res = res.replace(QRegExp("\\\\P"), "\n");
    // Space:
    res = res.replace(QRegExp("\\\\~"), " ");
    // Tab:
    res = res.replace(QRegExp("\\^I"), "    ");//RLZ: change 4 spaces for \t when mtext have support for tab
    // diameter:
    res = res.replace(QRegExp("%%[cC]"), QChar(0x2300));//RLZ: Empty_set is 0x2205, diameter is 0x2300 need to add in all fonts
    // degree:
    res = res.replace(QRegExp("%%[dD]"), QChar(0x00B0));
    // plus/minus
    res = res.replace(QRegExp("%%[pP]"), QChar(0x00B1));

    return res;
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
    //udpate (insert) after read
    update();

    if (success) {
        qInfo() << "Total layers" << dxf_layers_.size();
        for (auto &layer : dxf_layers_) {
            for(auto shape : layer->children()) {
                QTransform temp_trans = QTransform();
                temp_trans = temp_trans.scale(10, 10);
                shape->applyTransform(temp_trans);
            }
            doc->addLayer(layer);
            svg_layers->push_back(layer);
        }
    }
    return success;
}

void dxf_iface::addLayer(const DRW_Layer& data) {
    for(int i = 0;i < dxf_layers_.size(); ++i) {
        if(dxf_layers_[i]->name() == QString::fromUtf8(data.name.c_str())) {
            return;
        }
    }
    LayerPtr new_layer = std::make_shared<Layer>();
    new_layer->setName(QString::fromStdString(data.name));
    dxf_layers_.push_back(new_layer);

    // std::cout << __func__ << " " << data.name << " " << data.color << std::endl;
}

void dxf_iface::addBlock(const DRW_Block& data) {
    current_block_ = QString(data.name.c_str());
    BlockData new_data;
    new_data.base_pt = QPointF(data.basePoint.x, data.basePoint.y);
    block2shape_map_.insert(current_block_, QList<ShapePtr>());
    block2data_map_.insert(current_block_, new_data);
}

void dxf_iface::addLine(const DRW_Line& data) {
    QString layName = toNativeString(QString::fromUtf8(data.layer.c_str()));
    LayerPtr target_layer;
    for(int i = 0;i < dxf_layers_.size(); ++i) {
        target_layer = dxf_layers_[i];
        if(target_layer->name() == layName) {
            break;
        } else if(i == dxf_layers_.size()-1) {
            target_layer = std::make_shared<Layer>();
            target_layer->setName(layName);
            dxf_layers_.push_back(target_layer);
        }
    }

    // std::cout << __func__ << " basePoint.x = " << data.basePoint.x << " basePoint.y = " << data.basePoint.y << std::endl;
    // std::cout << __func__ << " secPoint.x = " << data.secPoint.x << " secPoint.y = " << data.secPoint.y << std::endl;
    QPainterPath working_path;
    working_path.moveTo(data.basePoint.x, data.basePoint.y);
    working_path.lineTo(data.secPoint.x, data.secPoint.y);
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    target_layer->addShape(new_shape);
    QMap<QString,QList<ShapePtr> >::iterator block_shape = block2shape_map_.find(current_block_);
    block_shape.value().push_back(new_shape);
}

void dxf_iface::addArc(const DRW_Arc& data) {
    QString layName = toNativeString(QString::fromUtf8(data.layer.c_str()));
    LayerPtr target_layer;
    for(int i = 0;i < dxf_layers_.size(); ++i) {
        target_layer = dxf_layers_[i];
        if(target_layer->name() == layName) {
            break;
        } else if(i == dxf_layers_.size()-1) {
            target_layer = std::make_shared<Layer>();
            target_layer->setName(layName);
            dxf_layers_.push_back(target_layer);
        }
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
    target_layer->addShape(new_shape);
    QMap<QString,QList<ShapePtr> >::iterator block_shape = block2shape_map_.find(current_block_);
    block_shape.value().push_back(new_shape);
}

void dxf_iface::addCircle(const DRW_Circle& data) {
    QString layName = toNativeString(QString::fromUtf8(data.layer.c_str()));
    LayerPtr target_layer;
    for(int i = 0;i < dxf_layers_.size(); ++i) {
        target_layer = dxf_layers_[i];
        if(target_layer->name() == layName) {
            break;
        } else if(i == dxf_layers_.size()-1) {
            target_layer = std::make_shared<Layer>();
            target_layer->setName(layName);
            dxf_layers_.push_back(target_layer);
        }
    }
    QPointF center(data.basePoint.x, data.basePoint.y);
    QPainterPath working_path;
    working_path.addEllipse(center, data.radious, data.radious);
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    target_layer->addShape(new_shape);
    QMap<QString,QList<ShapePtr> >::iterator block_shape = block2shape_map_.find(current_block_);
    block_shape.value().push_back(new_shape);
}

void dxf_iface::addLWPolyline(const DRW_LWPolyline& data) {
    // std::cout << __func__ << " " << __LINE__ << std::endl;
    if (data.vertlist.empty())
        return;
    QString layName = toNativeString(QString::fromUtf8(data.layer.c_str()));
    LayerPtr target_layer;
    for(int i = 0;i < dxf_layers_.size(); ++i) {
        target_layer = dxf_layers_[i];
        if(target_layer->name() == layName) {
            break;
        } else if(i == dxf_layers_.size()-1) {
            target_layer = std::make_shared<Layer>();
            target_layer->setName(layName);
            dxf_layers_.push_back(target_layer);
        }
    }
    QPainterPath working_path;
    working_path.moveTo(data.vertlist[0]->x, data.vertlist[0]->y);
    RS_Vector previous_pt(data.vertlist[0]->x, data.vertlist[0]->y);
    double next_bulge = data.vertlist[1]->bulge;
    for(unsigned int i = 1; i < data.vertlist.size(); ++i) {
        if(fabs(next_bulge) < 1.0e-10) {
            working_path.lineTo(data.vertlist[i]->x, data.vertlist[i]->y);
        } else { // create arc for the polyline:
            RS_Vector last_pt(data.vertlist[i]->x, data.vertlist[i]->y);
            double alpha = atan(next_bulge)*4;
            RS_Vector middle_pt((last_pt + previous_pt)/2.0);
            double dist = last_pt.distanceTo(previous_pt)/2.0;
            double angle = last_pt.angleTo(previous_pt);
            double radius = fabs(dist / sin(alpha/2));
            double h = sqrt(fabs(radius*radius - dist*dist));
            if(next_bulge >0) angle-=M_PI_2;
            else angle+=M_PI_2;
            if(fabs(alpha)>M_PI) h *= -1;
            RS_Vector center = RS_Vector::polar(h, angle);
            center += middle_pt;
            double staangle, endangle;
            if(next_bulge <0) {
                staangle = center.angleTo(last_pt);
                endangle = center.angleTo(previous_pt);
            } else {
                staangle = center.angleTo(previous_pt);
                endangle = center.angleTo(last_pt);
            }

            DRW_Arc new_arc;
            new_arc.layer = data.layer;
            new_arc.basePoint.x = center.x;
            new_arc.basePoint.y = center.y;
            new_arc.radious = radius;
            new_arc.staangle = staangle;
            new_arc.endangle = endangle;
            addArc(new_arc);
            previous_pt = RS_Vector(data.vertlist[i]->x, data.vertlist[i]->y);
        }
        next_bulge = data.vertlist[i]->bulge;
    }
    if(data.flags&0x1) {
        working_path.lineTo(data.vertlist[0]->x, data.vertlist[0]->y);
    }
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    target_layer->addShape(new_shape);
    QMap<QString,QList<ShapePtr> >::iterator block_shape = block2shape_map_.find(current_block_);
    block_shape.value().push_back(new_shape);
}

void dxf_iface::addPolyline(const DRW_Polyline& data) {
    if ( data.flags&0x10)
        return; //the polyline is a polygon mesh, not handled
    if ( data.flags&0x40)
        return; //the polyline is a poliface mesh, TODO convert
    QString layName = toNativeString(QString::fromUtf8(data.layer.c_str()));
    LayerPtr target_layer;
    for(int i = 0;i < dxf_layers_.size(); ++i) {
        target_layer = dxf_layers_[i];
        if(target_layer->name() == layName) {
            break;
        } else if(i == dxf_layers_.size()-1) {
            target_layer = std::make_shared<Layer>();
            target_layer->setName(layName);
            dxf_layers_.push_back(target_layer);
        }
    }
    QPainterPath working_path;
    working_path.moveTo(data.vertlist[0]->basePoint.x, data.vertlist[0]->basePoint.y);
    for(unsigned int i = 1; i < data.vertlist.size(); ++i) {
        // std::cout << data.vertlist[i]->basePoint.x << " " << data.vertlist[i]->basePoint.y << std::endl;
        working_path.lineTo(data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y);
    }
    if(data.flags&0x1) {
        working_path.lineTo(data.vertlist[0]->basePoint.x, data.vertlist[0]->basePoint.y);
    }
    ShapePtr new_shape = std::make_shared<PathShape>(working_path);
    target_layer->addShape(new_shape);
    QMap<QString,QList<ShapePtr> >::iterator block_shape = block2shape_map_.find(current_block_);
    block_shape.value().push_back(new_shape);
}

void dxf_iface::addSpline(const DRW_Spline* data) {
    QString layName = toNativeString(QString::fromUtf8(data->layer.c_str()));
    LayerPtr target_layer;
    for(int i = 0;i < dxf_layers_.size(); ++i) {
        target_layer = dxf_layers_[i];
        if(target_layer->name() == layName) {
            break;
        } else if(i == dxf_layers_.size()-1) {
            target_layer = std::make_shared<Layer>();
            target_layer->setName(layName);
            dxf_layers_.push_back(target_layer);
        }
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
    target_layer->addShape(new_shape);
    QMap<QString,QList<ShapePtr> >::iterator block_shape = block2shape_map_.find(current_block_);
    block_shape.value().push_back(new_shape);
    // std::cout << "controllist(" << data->ncontrol << "):" << std::endl;
    // int degree = data->degree;
    // std::cout << "degree = " << degree << std::endl;
    // for(unsigned int i = 0; i < data->controllist.size(); ++i)
    // {
    //     std::cout << data->controllist[i]->x << " " << data->controllist[i]->y << std::endl;
    // }
}

void dxf_iface::addVport(const DRW_Vport& data) {
    QString name = QString::fromStdString(data.name);
    if (name.toLower() == "*active") {
        view_height_ = data.height;
        view_width_ = data.height * data.ratio;
    }
}

void dxf_iface::addInsert(const DRW_Insert& data) {
    try {
        InsertData insert_data;
        insert_data.name = QString(data.name.c_str());
        insert_data.move_pt = QPointF(data.basePoint.x, data.basePoint.y);
        insert_data.colcount = data.colcount;
        insert_data.rowcount = data.rowcount;
        insert_data.colspace = data.colspace;
        insert_data.rowspace = data.rowspace;
        insert_data.scale_x = data.xscale;
        insert_data.scale_y = data.yscale;
        insert_data.angle = data.angle;
        insert_list_.push_back(insert_data);
    } catch(std::exception const &e) {
        std::cout << "Error connection: " << e.what();
    }
}

void dxf_iface::update() {
    try {
    for(unsigned int i = 0; i < insert_list_.size(); ++i) {
        QString block_name = insert_list_[i].name;
        if(block2data_map_.find(block_name) == block2data_map_.end() ||
            block2shape_map_.find(block_name) == block2shape_map_.end())
            continue;
        BlockData block_data = block2data_map_.find(block_name).value();
        QList<ShapePtr> new_list;
        for(auto shape : block2shape_map_.find(block_name).value()) {
            new_list.push_back(shape->clone());
            shape->layer()->addShape(new_list.last());
        }
        //update
        for (int c=0; c<insert_list_[i].colcount; ++c) {
        for (int r=0; r<insert_list_[i].rowcount; ++r) {
            for(auto shape : new_list) {
                QTransform temp_trans = QTransform();
                temp_trans = temp_trans.translate(-1*block_data.base_pt.x(), 
                                                    -1*block_data.base_pt.y());
                temp_trans = temp_trans.scale(insert_list_[i].scale_x,
                                                insert_list_[i].scale_y);
                temp_trans = temp_trans.rotate(qRadiansToDegrees(insert_list_[i].angle));
                shape->applyTransform(temp_trans);
            }
            if (fabs(insert_list_[i].scale_x)>1.0e-6 &&
                fabs(insert_list_[i].scale_y)>1.0e-6) {
                for(auto shape : new_list) {
                    QTransform move_trans = QTransform();
                    double move_x = insert_list_[i].move_pt.x() + 
                                    insert_list_[i].colspace / 
                                    insert_list_[i].scale_x*c;
                    double move_y = insert_list_[i].move_pt.y() + 
                                    insert_list_[i].rowspace / 
                                    insert_list_[i].scale_y*r;
                    shape->applyTransform(move_trans.translate(move_x, move_y));
                }
            } else {
                for(auto shape : new_list) {
                    QTransform move_trans = QTransform();
                    shape->applyTransform(move_trans.translate(insert_list_[i].move_pt.x(), 
                                                                insert_list_[i].move_pt.y()));
                }
            }
        }}
    }
    } catch(std::exception const &e) {
        std::cout << "Error connection: " << e.what();
    }
}